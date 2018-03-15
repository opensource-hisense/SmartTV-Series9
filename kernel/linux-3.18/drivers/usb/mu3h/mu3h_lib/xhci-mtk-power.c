#include "xhci-mtk-power.h"
#include "xhci.h"
#include "xhci-mtk.h"
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>
#include <linux/delay.h>


#define CON_HOST_DEV	0

int g_num_u3_port;
int g_num_u2_port;

#if CON_HOST_DEV
//filter those ports assigned to device
int getU3PortNumber(){
	int port_num;
	int real_port_num;
	int i, temp;
	
	//check if any port assigned to device
	port_num = SSUSB_U3_PORT_NUM(readl(SSUSB_IP_CAP));
	real_port_num = port_num;
	for(i=0; i<port_num; i++){
		temp = readl(SSUSB_U3_CTRL(i));
		if(!(temp & SSUSB_U3_PORT_HOST_SEL)){
			real_port_num--;
		}
	}
	return real_port_num;
}

//filter those ports assigned to device
int getU2PortNumber(){
	int port_num;
	int real_port_num;
	int i, temp;
	
	//check if any port assigned to device
	port_num = SSUSB_U2_PORT_NUM(readl(SSUSB_IP_CAP));
	real_port_num = port_num;
	for(i=0; i<port_num; i++){
		temp = readl(SSUSB_U2_CTRL(i));
		if(!(temp & SSUSB_U2_PORT_HOST_SEL)){
			real_port_num--;
		}
	}
	return real_port_num;
}

//filter those ports assigned to device
int getRealPortIndex(int port_index, int port_rev){
	int real_port_index, tmp_port_index;
	int i, temp;
	int portNum;

	real_port_index = 0;
	tmp_port_index = 0;
	if(port_rev == 0x3){
		//SS port
		portNum = getU3PortNumber();
		for(i=0; i<portNum; i++){
			temp = SSUSB_U3_CTRL(i);
			tmp_port_index++;
			if(temp & SSUSB_U3_PORT_HOST_SEL){
				real_port_index++;
				if(real_port_index == port_index){
					return tmp_port_index;
				}
			}
		}
	}
	else{
		//HS port
		portNum = getU2PortNumber();
		for(i=0; i<portNum; i++){
			temp = SSUSB_U2_CTRL(i);
			tmp_port_index++;
			if(temp & SSUSB_U2_PORT_HOST_SEL){
				real_port_index++;
				if(real_port_index == port_index){
					return tmp_port_index;
				}
			}
		}
	}
	return port_index;
}

#endif

void enableXhciPortPower(struct xhci_hcd *xhci, int port_id){
	u32 temp;
	u32 __iomem *addr;

	addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id - 1) & 0xff);
	temp = xhci_readl(xhci, addr);
	temp = xhci_port_state_to_neutral(temp);
	temp |= PORT_POWER;
	xhci_writel(xhci, temp, addr);

}

/* set 1 to PORT_POWER of PORT_STATUS register of each port */
void enableXhciAllPortPower(struct xhci_hcd *xhci){
	int i;
	u32 port_id, temp;
	u32 __iomem *addr;
	struct usb_hcd *hcd;
//	struct mtk_u3h_hw *u3h_hw;

	hcd = xhci_to_hcd(xhci);
	

#if CON_HOST_DEV
	g_num_u3_port = getU3PortNumber();
	g_num_u2_port = getU2PortNumber();
#else
	g_num_u3_port = get_xhci_u3_port_num(hcd->self.controller);
	g_num_u2_port = get_xhci_u2_port_num(hcd->self.controller);
#endif
	for(i=1; i<=g_num_u3_port; i++){
		port_id=i;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id - 1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(xhci, temp, addr);
	}
	for(i=1; i<=g_num_u2_port; i++){
		port_id=i+g_num_u3_port;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id - 1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(xhci, temp, addr);
	}
}

#ifdef SSM_ENABLE

void setIPPCreg(struct device *dev)
{
#if (SSM_PROJECT == MT7662T)
	struct mtk_u3h_hw *u3h_hw;
	void __iomem *addr;
    addr = u3h_hw->ippc_virtual_base + U3H_SSUSB_SYS_CK_CTRL;
	u3h_clrmsk(addr, SSUSB_SYS_CK_DIV2_EN);
#endif
}

#else
void setIPPCreg(struct device *dev) {}
#endif
void enableAllClockPower(struct device *dev){

	int i;
	u32 temp, temp_mask;
	int u3_port_num;
	int u2_port_num;	
	struct mtk_u3h_hw *u3h_hw;
	u32 __iomem *addr;
	
	u32 u4TimeoutCount = 100;

	u3h_hw = dev->platform_data;
	
	u3_port_num = get_xhci_u3_port_num(dev);
	u2_port_num = get_xhci_u2_port_num(dev);

	//1.	Enable xHC
#if OTG_SUPPORT
    addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_CTRL0);
    u3h_writelmsk(addr,0,SSUSB_IP_SW_RST);
	
	addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_CTRL1);
    u3h_writelmsk(addr,0,SSUSB_IP_HOST_PDN);
#else
    addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_CTRL0);
    u3h_writelmsk(addr,1,SSUSB_IP_SW_RST);

    addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_CTRL0);
    u3h_writelmsk(addr,0,SSUSB_IP_SW_RST);
	
	addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_CTRL1);
    u3h_writelmsk(addr,0,SSUSB_IP_HOST_PDN);
#endif	

	
	//2.	Enable target ports 
	for(i=0; i<u3_port_num; i++){
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U3_CTRL_0P + i*0x08);
		temp = readl(addr);
#if CON_HOST_DEV
		if(temp & SSUSB_U3_PORT_HOST_SEL){
			temp = temp & (~SSUSB_U3_PORT_PDN) & (~SSUSB_U3_PORT_DIS);
		}
#else
		temp = temp & (~SSUSB_U3_PORT_PDN) & (~SSUSB_U3_PORT_DIS);
#endif
		
		writel(temp, addr);
	}
	
	for(i=0; i<u2_port_num; i++){		
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U2_CTRL_0P + i*0x08);
		temp = readl(addr);		
#if CON_HOST_DEV		
		if(temp & SSUSB_U2_PORT_HOST_SEL){
			temp = temp & (~SSUSB_U2_PORT_PDN) & (~SSUSB_U2_PORT_DIS);
		}
#else
#if OTG_SUPPORT
		temp = temp & (~SSUSB_U2_PORT_PDN) & (~SSUSB_U2_PORT_DIS) & (~SSUSB_U2_PORT_HOST_SEL);
		temp = temp | SSUSB_U2_PORT_OTG_SEL | SSUSB_U2_PORT_OTG_MAC_AUTO_SEL | SSUSB_U2_PORT_HOST_SEL;
#else
		temp = temp & (~SSUSB_U2_PORT_PDN) & (~SSUSB_U2_PORT_DIS);
#endif

#endif
		writel(temp, addr);

	}

	//wait clock stable
	temp_mask = (SSUSB_SYSPLL_STABLE | SSUSB_XHCI_RST_B_STS | SSUSB_U3_MAC_RST_B_STS);	
	do
	{		
        addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_STS1);
		temp = readl(addr);				
		if ((temp & temp_mask) == temp_mask)
		{			
		    break;		
		}       
		msleep(1);	
	}while(u4TimeoutCount--);
	//	msleep(100);
}

//called after HC initiated
void disableAllClockPower(struct device *dev){
	int i;
	u32 temp;
	int u3_port_num;
	int u2_port_num;	
	struct mtk_u3h_hw *u3h_hw;
	u32 __iomem *addr;


#if OTG_SUPPORT
		return;
#endif

	u3h_hw = dev->platform_data;
	
	u3_port_num = get_xhci_u3_port_num(dev);
	u2_port_num = get_xhci_u2_port_num(dev);

	//disable target ports
	for(i=0; i<u3_port_num; i++){
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U3_CTRL_0P + i*0x08);
		temp = readl(addr);
#if CON_HOST_DEV
		if(temp & SSUSB_U3_PORT_HOST_SEL){
			temp = temp | SSUSB_U3_PORT_PDN;
		}
#else
		temp = temp | SSUSB_U3_PORT_PDN;
#endif
		writel(temp, addr);
	}
	for(i=0; i<u2_port_num; i++){
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U2_CTRL_0P + i*0x08);
		temp = readl(addr);
#if CON_HOST_DEV
		if(temp & SSUSB_U2_PORT_HOST_SEL){
			temp = temp | SSUSB_U2_PORT_PDN;
		}
#else
		temp = temp | SSUSB_U2_PORT_PDN;
#endif
		writel(temp, addr);
	}
	msleep(100);
}

//(X)disable clock/power of a port 
//(X)if all ports are disabled, disable IP ctrl power
//disable all ports and IP clock/power, this is just mention HW that the power/clock of port 
//and IP could be disable if suspended.
//If doesn't not disable all ports at first, the IP clock/power will never be disabled
//(some U2 and U3 ports are binded to the same connection, that is, they will never enter suspend at the same time
//port_index: port number
//port_rev: 0x2 - USB2.0, 0x3 - USB3.0 (SuperSpeed)

void disablePortClockPower(struct device *dev, int port_index, int port_rev){
//	u32 temp;
	int real_index;
	
	struct mtk_u3h_hw *u3h_hw;
	u32 __iomem *addr;

#if OTG_SUPPORT
	return;
#endif

#if CON_HOST_DEV
	real_index = getRealPortIndex(port_index, port_rev);
#else
	real_index = port_index;
#endif

	u3h_hw = dev->platform_data;

	if(port_rev == 0x3){
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U3_CTRL_0P + (real_index - 1) * 0x08);
		u3h_writelmsk(addr,0x1<<1,SSUSB_U3_PORT_PDN);
	}
	else if(port_rev == 0x2){
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U2_CTRL_0P + (real_index - 1) * 0x08);
		u3h_writelmsk(addr,0x1<<1,SSUSB_U2_PORT_PDN);
	}
	addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_CTRL1);
	u3h_writelmsk(addr,0x1<<0,SSUSB_IP_HOST_PDN);
}

//if IP ctrl power is disabled, enable it
//enable clock/power of a port
//port_index: port number
//port_rev: 0x2 - USB2.0, 0x3 - USB3.0 (SuperSpeed)
void enablePortClockPower(struct device *dev, int port_index, int port_rev){
//	u32 temp;
	int real_index;
	
	struct mtk_u3h_hw *u3h_hw;
	u32 __iomem *addr;

#if OTG_SUPPORT
	return;
#endif	
#if CON_HOST_DEV
	real_index = getRealPortIndex(port_index, port_rev);
#else
	real_index = port_index;
#endif

	u3h_hw = dev->platform_data;

	addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_IP_PW_CTRL1);
	u3h_writelmsk(addr,~(0x1<<0),SSUSB_IP_HOST_PDN);

	if(port_rev == 0x3){
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U3_CTRL_0P + (real_index - 1) * 0x08);
		u3h_writelmsk(addr,~(0x1<<1),SSUSB_U3_PORT_PDN);
	}
	else if(port_rev == 0x2){
		addr = (u32 __iomem *)(u3h_hw->ippc_virtual_base + U3H_SSUSB_U2_CTRL_0P + (real_index - 1) * 0x08);
		u3h_writelmsk(addr,~(0x1<<1),SSUSB_U2_PORT_PDN);
	}
}
