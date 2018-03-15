#include "xhci-mtk-scheduler.h"
#include <linux/slab.h>
#include "xhci-mtk.h"



int mtk_xhci_scheduler_init(struct device *dev){
	int i,j;
	struct mtk_u3h_hw *u3h_hw;
	struct sch_port *tem_sch_port;

	u3h_hw = dev->platform_data;

	for (i=0; i<MAX_PORT_NUM; i++){
		tem_sch_port = &(u3h_hw->u3h_sch_port[i]);
		for(j=0; j<MAX_EP_NUM; j++){
			tem_sch_port->ss_out_eps[j] = NULL;
			tem_sch_port->ss_in_eps[j] = NULL;
			tem_sch_port->hs_eps[j] = NULL;
			tem_sch_port->tt_intr_eps[j] = NULL;
		}
	}

	return 0;
}

int add_sch_ep(int rh_port_num, int dev_speed, int is_in, int isTT, int ep_type, int maxp, int interval, int burst
	, int mult, int offset, int repeat, int pkts, int cs_count, int burst_mode
	, int bw_cost, mtk_u32 *ep, struct sch_port *u3h_sch_port){

	struct sch_ep **ep_array;
	struct sch_ep *u3h_sch_ep;
	int i;

	u3h_sch_ep = kmalloc(sizeof(struct sch_ep), GFP_KERNEL);

	if(is_in && dev_speed == USB_SPEED_SUPER ){
		ep_array = (struct sch_ep **)(u3h_sch_port->ss_in_eps);
	}
	else if(dev_speed == USB_SPEED_SUPER){
		ep_array = (struct sch_ep **)(u3h_sch_port->ss_out_eps);
	}
	else if(dev_speed == USB_SPEED_HIGH || (isTT && ep_type == USB_EP_ISOC)){
		ep_array = (struct sch_ep **)(u3h_sch_port->hs_eps);
	}
	else{
		ep_array = (struct sch_ep **)(u3h_sch_port->tt_intr_eps);
	}
	for(i=0; i<MAX_EP_NUM; i++){
		if(ep_array[i] == NULL){
			u3h_sch_ep->rh_port_num = rh_port_num;
			u3h_sch_ep->dev_speed = dev_speed;
			u3h_sch_ep->isTT = isTT;
			u3h_sch_ep->is_in = is_in;
			u3h_sch_ep->ep_type = ep_type;
			u3h_sch_ep->maxp = maxp;
			u3h_sch_ep->interval = interval;
			u3h_sch_ep->burst = burst;
			u3h_sch_ep->mult = mult;
			u3h_sch_ep->offset = offset;
			u3h_sch_ep->repeat = repeat;
			u3h_sch_ep->pkts = pkts;
			u3h_sch_ep->cs_count = cs_count;
			u3h_sch_ep->burst_mode = burst_mode;
			u3h_sch_ep->bw_cost = bw_cost;
			u3h_sch_ep->ep = ep;
			ep_array[i] = u3h_sch_ep;
			return SCH_SUCCESS;
		}
	}

	kfree(u3h_sch_ep);
	return SCH_FAIL;
}

int count_ss_bw(int is_in, int ep_type, int maxp, int interval, int burst, int mult, int offset, int repeat
	, int td_size, struct sch_port *u3h_sch_port){
	int i, j, k;
	int bw_required[3];
	int final_bw_required;
	int bw_required_per_repeat;
	int tmp_bw_required;
	struct sch_ep *cur_sch_ep;
	struct sch_ep **ep_array;
	int cur_offset;
	int cur_ep_offset;
	int tmp_offset;
	int tmp_interval;
	int ep_offset;
	int ep_interval;
	int ep_repeat;
	int ep_mult;

	if(is_in){
		ep_array = (struct sch_ep **)(u3h_sch_port->ss_in_eps);
	}
	else{
		ep_array = (struct sch_ep **)(u3h_sch_port->ss_out_eps);
	}

	bw_required[0] = 0;
	bw_required[1] = 0;
	bw_required[2] = 0;

	if(repeat == 0){
		final_bw_required = 0;
		for(i=0; i<MAX_EP_NUM; i++){
			cur_sch_ep = ep_array[i];
			if(cur_sch_ep == NULL){
				continue;
			}
			ep_interval = cur_sch_ep->interval;
			ep_offset = cur_sch_ep->offset;
			if(cur_sch_ep->repeat == 0){
				if(ep_interval >= interval){
					tmp_offset = ep_offset + ep_interval - offset;
					tmp_interval = interval;
				}
				else{
					tmp_offset = offset + interval - ep_offset;
					tmp_interval = ep_interval;
				}
				if(tmp_offset % tmp_interval == 0){
					final_bw_required += cur_sch_ep->bw_cost;
				}
			}
			else{
				ep_repeat = cur_sch_ep->repeat;
				ep_mult = cur_sch_ep->mult;
				for(k=0; k<=ep_mult; k++){
					cur_ep_offset = ep_offset+(k*ep_repeat);
					if(ep_interval >= interval){
						tmp_offset = cur_ep_offset + ep_interval - offset;
						tmp_interval = interval;
					}
					else{
						tmp_offset = offset + interval - cur_ep_offset;
						tmp_interval = ep_interval;
					}
					if(tmp_offset % tmp_interval == 0){
						final_bw_required += cur_sch_ep->bw_cost;
						break;
					}
				}
			}
		}
		final_bw_required += td_size;
	}
	else{
		bw_required_per_repeat = maxp * (burst+1);
		for(j=0; j<=mult; j++){
			tmp_bw_required = 0;
			cur_offset = offset+(j*repeat);
			for(i=0; i<MAX_EP_NUM; i++){
				cur_sch_ep = ep_array[i];
				if(cur_sch_ep == NULL){
					continue;
				}
				ep_interval = cur_sch_ep->interval;
				ep_offset = cur_sch_ep->offset;
				if(cur_sch_ep->repeat == 0){
					if(ep_interval >= interval){
						tmp_offset = ep_offset + ep_interval - cur_offset;
						tmp_interval = interval;
					}
					else{
						tmp_offset = cur_offset + interval - ep_offset;
						tmp_interval = ep_interval;
					}
					if(tmp_offset % tmp_interval == 0){
						tmp_bw_required += cur_sch_ep->bw_cost;
					}
				}
				else{
					ep_repeat = cur_sch_ep->repeat;
					ep_mult = cur_sch_ep->mult;
					for(k=0; k<=ep_mult; k++){
						cur_ep_offset = ep_offset+(k*ep_repeat);
						if(ep_interval >= interval){
							tmp_offset = cur_ep_offset + ep_interval - cur_offset;
							tmp_interval = interval;
						}
						else{
							tmp_offset = cur_offset + interval - cur_ep_offset;
							tmp_interval = ep_interval;
						}
						if(tmp_offset % tmp_interval == 0){
							tmp_bw_required += cur_sch_ep->bw_cost;
							break;
						}
					}
				}
			}
			bw_required[j] = tmp_bw_required;
		}
		final_bw_required = SS_BW_BOUND;
		for(j=0; j<=mult; j++){
			if(bw_required[j] < final_bw_required){
				final_bw_required = bw_required[j];
			}
		}
		final_bw_required += bw_required_per_repeat;
	}
	return final_bw_required;
}

int count_hs_bw(int ep_type, int maxp, int interval, int offset, int td_size, struct sch_port *u3h_sch_port){
	int i;
	int bw_required;
	struct sch_ep *cur_sch_ep;
	int tmp_offset;
	int tmp_interval;
	int ep_offset;
	int ep_interval;
	int cur_tt_isoc_interval;	//for isoc tt check

	bw_required = 0;
	for(i=0; i<MAX_EP_NUM; i++){

		cur_sch_ep = (struct sch_ep *)(u3h_sch_port->hs_eps[i]);
		if(cur_sch_ep == NULL){
				continue;
		}
		ep_offset = cur_sch_ep->offset;
		ep_interval = cur_sch_ep->interval;

		if(cur_sch_ep->isTT && cur_sch_ep->ep_type == USB_EP_ISOC){
			cur_tt_isoc_interval = ep_interval<<3;
			if(ep_interval >= interval){
				tmp_offset = ep_offset + cur_tt_isoc_interval - offset;
				tmp_interval = interval;
			}
			else{
				tmp_offset = offset + interval - ep_offset;
				tmp_interval = cur_tt_isoc_interval;
			}
			if(cur_sch_ep->is_in){
				if((tmp_offset%tmp_interval >=2) && (tmp_offset%tmp_interval <= cur_sch_ep->cs_count)){
					bw_required += 188;
				}
			}
			else{
				if(tmp_offset%tmp_interval <= cur_sch_ep->cs_count){
					bw_required += 188;
				}
			}
		}
		else{
			if(ep_interval >= interval){
				tmp_offset = ep_offset + ep_interval - offset;
				tmp_interval = interval;
			}
			else{
				tmp_offset = offset + interval - ep_offset;
				tmp_interval = ep_interval;
			}
			if(tmp_offset%tmp_interval == 0){
				bw_required += cur_sch_ep->bw_cost;
			}
		}
	}
	bw_required += td_size;
	return bw_required;
}

int count_tt_isoc_bw(int is_in, int maxp, int interval, int offset, int td_size, struct sch_port *u3h_sch_port){
	char is_cs;
	int s_frame, s_mframe, cur_mframe;
	int bw_required, max_bw;
	int ss_cs_count;
	int cs_mframe;
//	int max_frame;
	int i,j;
	struct sch_ep *cur_sch_ep;
	int ep_offset;
	int ep_interval;
//	int ep_cs_count;
	int tt_isoc_interval;	//for isoc tt check
	int cur_tt_isoc_interval;	//for isoc tt check
	int tmp_offset;
	int tmp_interval;

	is_cs = 0;

	tt_isoc_interval = interval<<3;	//frame to mframe
	if(is_in){
		is_cs = 1;
	}
	s_frame = offset/8;
	s_mframe = offset%8;
	ss_cs_count = (maxp + (188 - 1))/188;
	if(is_cs){
		cs_mframe = offset%8 + 2 + ss_cs_count;
		if (cs_mframe <= 6)
			ss_cs_count += 2;
		else if (cs_mframe == 7)
			ss_cs_count++;
		else if (cs_mframe > 8)
			return -1;
	}
	max_bw = 0;
	if(is_in){
		i=2;
	}
	for(cur_mframe = offset+i; i<ss_cs_count; cur_mframe++, i++){
		bw_required = 0;
		for(j=0; j<MAX_EP_NUM; j++){
			cur_sch_ep = (struct sch_ep *)(u3h_sch_port->hs_eps[j]);
			if(cur_sch_ep == NULL){
				continue;
			}
			ep_offset = cur_sch_ep->offset;
			ep_interval = cur_sch_ep->interval;
			if(cur_sch_ep->isTT && cur_sch_ep->ep_type == USB_EP_ISOC){
				//isoc tt
				//check if mframe offset overlap
				//if overlap, add 188 to the bw
				cur_tt_isoc_interval = ep_interval<<3;
				if(cur_tt_isoc_interval >= tt_isoc_interval){
					tmp_offset = (ep_offset+cur_tt_isoc_interval)  - cur_mframe;
					tmp_interval = tt_isoc_interval;
				}
				else{
					tmp_offset = (cur_mframe+tt_isoc_interval) - ep_offset;
					tmp_interval = cur_tt_isoc_interval;
				}
				if(cur_sch_ep->is_in){
					if((tmp_offset%tmp_interval >=2) && (tmp_offset%tmp_interval <= cur_sch_ep->cs_count)){
						bw_required += 188;
					}
				}
				else{
					if(tmp_offset%tmp_interval <= cur_sch_ep->cs_count){
						bw_required += 188;
					}
				}

			}
			else if(cur_sch_ep->ep_type == USB_EP_INT || cur_sch_ep->ep_type == USB_EP_ISOC){
				//check if mframe
				if(ep_interval >= tt_isoc_interval){
					tmp_offset = (ep_offset+ep_interval) - cur_mframe;
					tmp_interval = tt_isoc_interval;
				}
				else{
					tmp_offset = (cur_mframe+tt_isoc_interval) - ep_offset;
					tmp_interval = ep_interval;
				}
				if(tmp_offset%tmp_interval == 0){
					bw_required += cur_sch_ep->bw_cost;
				}
			}
		}
		bw_required += 188;
		if(bw_required > max_bw){
			max_bw = bw_required;
		}
	}
	return max_bw;
}

int count_tt_intr_bw(int interval, int frame_offset, struct sch_port *u3h_sch_port){
	//check all eps in tt_intr_eps
	int ret;
	int i;
	int ep_offset;
	int ep_interval;
	int tmp_offset;
	int tmp_interval;
	struct sch_ep *cur_sch_ep;

	ret = SCH_SUCCESS;

	for(i=0; i<MAX_EP_NUM; i++){
		cur_sch_ep = (struct sch_ep *)(u3h_sch_port->tt_intr_eps[i]);
		if(cur_sch_ep == NULL){
			continue;
		}
		ep_offset = cur_sch_ep->offset;
		ep_interval = cur_sch_ep->interval;
		if(ep_interval  >= interval){
			tmp_offset = ep_offset + ep_interval - frame_offset;
			tmp_interval = interval;
		}
		else{
			tmp_offset = frame_offset + interval - ep_offset;
			tmp_interval = ep_interval;
		}
		if(tmp_offset%tmp_interval==0){
			return SCH_FAIL;
		}
	}
	return SCH_SUCCESS;
}

int mtk_xhci_scheduler_remove_ep(struct usb_hcd *hcd, struct usb_device *udev, struct usb_host_endpoint *ep){
	int i;
	int is_in, rh_port_num, dev_speed,ep_type,isTT;
	struct sch_ep **ep_array;
	struct sch_ep *cur_ep;

	struct sch_port *u3h_sch_port;
	struct mtk_u3h_hw *u3h_hw;
	struct device *dev;

	struct xhci_hcd *xhci;
	struct xhci_slot_ctx *slot_ctx;
	struct xhci_virt_device *virt_dev;

	if(usb_endpoint_xfer_int(&ep->desc)){
		ep_type = USB_EP_INT;
	}
	else if(usb_endpoint_xfer_isoc(&ep->desc)){
		ep_type = USB_EP_ISOC;
	}
	else if(usb_endpoint_xfer_bulk(&ep->desc)){
		ep_type = USB_EP_BULK;
	}
	else {
		ep_type = USB_EP_CONTROL;
	}

	if (ep_type == USB_EP_CONTROL || ep_type == USB_EP_BULK){
		return SCH_SUCCESS;
	}

	xhci = hcd_to_xhci(hcd);
	virt_dev = xhci->devs[udev->slot_id];
	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->in_ctx);

	is_in = usb_endpoint_dir_in(&ep->desc);
	rh_port_num = (slot_ctx->dev_info2 >> 16) & 0xff;
	dev_speed = udev->speed;

	if((slot_ctx->tt_info & 0xff) > 0){
		isTT = 1;
	}
	else{
		isTT = 0;
	}

	if(isTT == 0 && (dev_speed == USB_SPEED_FULL || dev_speed == USB_SPEED_LOW)){
		return SCH_SUCCESS;
	}

	dev = hcd->self.controller;
	u3h_hw = dev->platform_data;
	u3h_sch_port = &(u3h_hw->u3h_sch_port[rh_port_num - 1]);

	if(is_in && dev_speed == USB_SPEED_SUPER){
		ep_array = (struct sch_ep **)(u3h_sch_port->ss_in_eps);
	}
	else if(dev_speed == USB_SPEED_SUPER){
		ep_array = (struct sch_ep **)(u3h_sch_port->ss_out_eps);
	}
	else if(dev_speed == USB_SPEED_HIGH || (isTT && ep_type == USB_EP_ISOC)){
		ep_array = (struct sch_ep **)(u3h_sch_port->hs_eps);
	}
	else{
		ep_array = (struct sch_ep **)(u3h_sch_port->tt_intr_eps);
	}
	for(i=0; i<MAX_EP_NUM; i++){
		cur_ep = (struct sch_ep *)ep_array[i];
		if(cur_ep != NULL && cur_ep->ep == (mtk_u32 *)ep){
			ep_array[i] = NULL;
			kfree(ep_array[i]);
			return SCH_SUCCESS;
		}
	}
	return SCH_FAIL;
}

int mtk_xhci_scheduler_add_ep(struct usb_hcd *hcd, struct usb_device *udev, struct usb_host_endpoint *ep){
	mtk_u32 bPkts = 0;
	mtk_u32 bCsCount = 0;
	mtk_u32 bBm = 1;
	mtk_u32 bOffset = 0;
	mtk_u32 bRepeat = 0;
	int ret;
	struct mtk_xhci_ep_ctx *temp_ep_ctx;
	int td_size;
	int mframe_idx, frame_idx;
	int bw_cost;
	int cur_bw, best_bw, best_bw_idx,repeat, max_repeat, best_bw_repeat;
	int cur_offset, cs_mframe;
	int break_out;
	int frame_interval;
	int rh_port_num, dev_speed, is_in, isTT, ep_type, maxp, interval, burst, mult;
	struct sch_port *u3h_sch_port;
	struct mtk_u3h_hw *u3h_hw;
	struct device *dev;

	struct xhci_hcd *xhci;
	struct xhci_container_ctx *in_ctx;
	unsigned int ep_index;
	struct xhci_ep_ctx *ep_ctx;
	struct xhci_slot_ctx *slot_ctx;
	struct xhci_virt_device *virt_dev;

	xhci = hcd_to_xhci(hcd);
	virt_dev = xhci->devs[udev->slot_id];
	in_ctx = virt_dev->in_ctx;
	ep_index = xhci_get_endpoint_index(&ep->desc);
	ep_ctx = xhci_get_ep_ctx(xhci, in_ctx, ep_index);
	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->in_ctx);

	if((slot_ctx->tt_info & 0xff) > 0){
		isTT = 1;
	}
	else{
		isTT = 0;
	}

	if(usb_endpoint_xfer_int(&ep->desc)){
		ep_type = USB_EP_INT;
	}
	else if(usb_endpoint_xfer_isoc(&ep->desc)){
		ep_type = USB_EP_ISOC;
	}
	else if(usb_endpoint_xfer_bulk(&ep->desc)){
		ep_type = USB_EP_BULK;
	}
	else {
		ep_type = USB_EP_CONTROL;
	}

	if(udev->speed == USB_SPEED_FULL || udev->speed == USB_SPEED_HIGH || udev->speed == USB_SPEED_LOW){
		maxp = ep->desc.wMaxPacketSize & 0x7FF;
		burst = ep->desc.wMaxPacketSize >> 11;
		mult = 0;
	}
	else if(udev->speed == USB_SPEED_SUPER){
		maxp = ep->desc.wMaxPacketSize & 0x7FF;
		burst = ep->ss_ep_comp.bMaxBurst;
		mult = ep->ss_ep_comp.bmAttributes & 0x3;
	}
	else{
		maxp = ep->desc.wMaxPacketSize & 0x7FF;
		burst = 0;
		mult = 0;
	}

	dev_speed = udev->speed;
	interval = (1 << ((ep_ctx->ep_info >> 16) & 0xff));
	is_in = usb_endpoint_dir_in(&ep->desc);
	rh_port_num = (slot_ctx->dev_info2 >> 16) & 0xff;

	dev = hcd->self.controller;
	u3h_hw = dev->platform_data;
	u3h_sch_port = &(u3h_hw->u3h_sch_port[rh_port_num - 1]);

	printk(KERN_DEBUG "add_ep parameters: rh_port_num %d, dev_speed %d, is_in %d, isTT %d, ep_type %d, maxp %d, interval %d, burst %d, mult %d, ep 0x%x, ep_ctx 0x%x\n", rh_port_num, dev_speed, is_in, isTT, ep_type, maxp
		, interval, burst, mult, (unsigned int)ep, (unsigned int)ep_ctx);
	if(isTT && ep_type == USB_EP_INT && ((dev_speed == USB_SPEED_LOW) || (dev_speed == USB_SPEED_FULL))){
		frame_interval = interval >> 3;
		for(frame_idx=0; frame_idx<frame_interval; frame_idx++){
			printk(KERN_DEBUG "check tt_intr_bw interval %d, frame_idx %d\n", frame_interval, frame_idx);
			if(count_tt_intr_bw(frame_interval, frame_idx, u3h_sch_port) == SCH_SUCCESS){
				printk(KERN_DEBUG "check OK............\n");
				bOffset = frame_idx<<3;
				bPkts = 1;
				bCsCount = 3;
				bw_cost = maxp;
				bRepeat = 0;
				if(add_sch_ep(rh_port_num, dev_speed, is_in, isTT, ep_type, maxp, frame_interval, burst, mult
					, bOffset, bRepeat, bPkts, bCsCount, bBm, maxp, (mtk_u32 *)ep,u3h_sch_port) == SCH_FAIL){
					return SCH_FAIL;
				}
				ret = SCH_SUCCESS;
				break;
			}
		}
	}
	else if(isTT && ep_type == USB_EP_ISOC){
		best_bw = HS_BW_BOUND;
		best_bw_idx = -1;
		cur_bw = 0;
		td_size = maxp;
		break_out = 0;
		frame_interval = interval>>3;
		for(frame_idx=0; frame_idx<frame_interval && !break_out; frame_idx++){
			for(mframe_idx=0; mframe_idx<8; mframe_idx++){
				cur_offset = (frame_idx*8) + mframe_idx;
				cur_bw = count_tt_isoc_bw(is_in, maxp, frame_interval, cur_offset, td_size, u3h_sch_port);
				if(cur_bw >= 0 && cur_bw < best_bw){
					best_bw_idx = cur_offset;
					best_bw = cur_bw;
					if(cur_bw == td_size || cur_bw < (HS_BW_BOUND>>1)){
						break_out = 1;
						break;
					}
				}
			}
		}
		if(best_bw_idx == -1){
			return SCH_FAIL;
		}
		else{
			bOffset = best_bw_idx;
			bPkts = 1;
			bCsCount = (maxp + (188 - 1)) / 188;
			if(is_in){
				cs_mframe = bOffset%8 + 2 + bCsCount;
				if (cs_mframe <= 6)
					bCsCount += 2;
				else if (cs_mframe == 7)
					bCsCount++;
			}
			bw_cost = 188;
			bRepeat = 0;
			if(add_sch_ep(rh_port_num, dev_speed, is_in, isTT, ep_type, maxp, interval, burst, mult
				, bOffset, bRepeat, bPkts, bCsCount, bBm, bw_cost, (mtk_u32 *)ep, u3h_sch_port) == SCH_FAIL){
				return SCH_FAIL;
			}
			ret = SCH_SUCCESS;
		}
	}
	else if((dev_speed == USB_SPEED_FULL || dev_speed == USB_SPEED_LOW) && ep_type == USB_EP_INT){
		bPkts = 1;
		ret = SCH_SUCCESS;
	}
	else if(dev_speed == USB_SPEED_FULL && ep_type == USB_EP_ISOC){
		bPkts = 1;
		ret = SCH_SUCCESS;
	}
	else if(dev_speed == USB_SPEED_HIGH && (ep_type == USB_EP_INT || ep_type == USB_EP_ISOC)){
		best_bw = HS_BW_BOUND;
		best_bw_idx = -1;
		cur_bw = 0;
		td_size = maxp*(burst+1);
		for(cur_offset = 0; cur_offset<interval; cur_offset++){
			cur_bw = count_hs_bw(ep_type, maxp, interval, cur_offset, td_size, u3h_sch_port);
			if(cur_bw >= 0 && cur_bw < best_bw){
				best_bw_idx = cur_offset;
				best_bw = cur_bw;
				if(cur_bw == td_size || cur_bw < (HS_BW_BOUND>>1)){
					break;
				}
			}
		}
		if(best_bw_idx == -1){
			return SCH_FAIL;
		}
		else{
			bOffset = best_bw_idx;
			bPkts = burst + 1;
			bCsCount = 0;
			bw_cost = td_size;
			bRepeat = 0;
			if(add_sch_ep(rh_port_num, dev_speed, is_in, isTT, ep_type, maxp, interval, burst, mult
				, bOffset, bRepeat, bPkts, bCsCount, bBm, bw_cost, (mtk_u32 *)ep, u3h_sch_port) == SCH_FAIL){
				return SCH_FAIL;
			}
			ret = SCH_SUCCESS;
		}
	}
	else if(dev_speed == USB_SPEED_SUPER && (ep_type == USB_EP_INT || ep_type == USB_EP_ISOC)){
		best_bw = SS_BW_BOUND;
		best_bw_idx = -1;
		cur_bw = 0;
		td_size = maxp * (mult+1) * (burst+1);
		if(mult == 0){
			max_repeat = 0;
		}
		else{
			max_repeat = (interval-1)/(mult+1);
		}
		break_out = 0;
		best_bw_repeat = 0;
		for(frame_idx = 0; (frame_idx < interval) && !break_out; frame_idx++){
			for(repeat = max_repeat; repeat >= 0; repeat--){
				cur_bw = count_ss_bw(is_in, ep_type, maxp, interval, burst, mult, frame_idx
					, repeat, td_size, u3h_sch_port);
				printk(KERN_DEBUG "count_ss_bw, frame_idx %d, repeat %d, td_size %d, result bw %d\n"
					, frame_idx, repeat, td_size, cur_bw);
				if(cur_bw >= 0 && cur_bw < best_bw){
					best_bw_idx = frame_idx;
					best_bw_repeat = repeat;
					best_bw = cur_bw;
					if(cur_bw <= td_size || cur_bw < (SS_BW_BOUND>>1)){
						break_out = 1;
						break;
					}
				}
			}
		}
		printk(KERN_DEBUG "final best idx %d, best repeat %d\n", best_bw_idx, best_bw_repeat);
		if(best_bw_idx == -1){
			return SCH_FAIL;
		}
		else{
			bOffset = best_bw_idx;
			bCsCount = 0;
			bRepeat = best_bw_repeat;
			if(bRepeat == 0){
				bw_cost = (burst+1)*(mult+1)*maxp;
				bPkts = (burst+1)*(mult+1);
			}
			else{
				bw_cost = (burst+1)*maxp;
				bPkts = (burst+1);
			}
			if(add_sch_ep(rh_port_num, dev_speed, is_in, isTT, ep_type, maxp, interval, burst, mult
				, bOffset, bRepeat, bPkts, bCsCount, bBm, bw_cost, (mtk_u32 *)ep, u3h_sch_port) == SCH_FAIL){
				return SCH_FAIL;
			}
			ret = SCH_SUCCESS;
		}
	}
	else{
		bPkts = 1;
		ret = SCH_SUCCESS;
	}
	if(ret == SCH_SUCCESS){
		temp_ep_ctx = (struct mtk_xhci_ep_ctx *)ep_ctx;
		temp_ep_ctx->reserved[0] |= (BPKTS(bPkts) | BCSCOUNT(bCsCount) | BBM(bBm));
		temp_ep_ctx->reserved[1] |= (BOFFSET(bOffset) | BREPEAT(bRepeat));
		printk(KERN_DEBUG "[DBG] BPKTS: %x, BCSCOUNT: %x, BBM: %x\n", bPkts, bCsCount, bBm);
		printk(KERN_DEBUG "[DBG] BOFFSET: %x, BREPEAT: %x\n", bOffset, bRepeat);
		//if(ep != NULL){
		//	printk(KERN_ERR "[DBG]free the sch_ep\n");
		//	kfree(ep);
		//}
		return SCH_SUCCESS;
	}
	else{
		return SCH_FAIL;
	}
}
