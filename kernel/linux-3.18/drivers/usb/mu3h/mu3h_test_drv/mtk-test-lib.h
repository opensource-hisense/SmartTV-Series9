#ifndef __MTK_TEST_LIB_H
#define __MTK_TEST_LIB_H
#include "mtk-usb-hcd.h"
#include "mtk-phy.h"
#include "xhci-mtk.h"

#define TEST_OTG	0	//if test OTG function, enable this

#if TEST_OTG
#define TEST_PET 1
#else 
#define TEST_PET 0
#endif

struct ixia_dev
{
	struct usb_device *udev;
	int ep_out;
	int ep_in;
};

int f_enable_port(int index);
int f_disconnect_port(int index);
int f_enable_slot(struct usb_device *dev);
int f_disable_slot(void);
int f_address_slot(char isBSR, struct usb_device *dev);
int f_slot_reset_device(int slot_id, char isWarmReset);
int f_udev_add_ep(struct usb_host_endpoint *ep, struct usb_device *udev);
int f_xhci_config_ep(struct usb_device *udev);
int f_evaluate_context(int max_exit_latency, int maxp0, int preping_mode, int preping, int besl, int besld);

int f_power_suspend(void);
int f_power_resume(void);
int f_power_remotewakeup(void);
int f_power_set_u1u2(int u_num, int value1, int value2);
int f_power_send_fla(int value);

int f_ring_stop_cmd(void);
int f_ring_abort_cmd(void);
int f_ring_enlarge(int ep_dir, int ep_num, int dev_num);
int f_ring_stop_ep(int slot_id, int ep_index);
int f_ring_set_tr_dequeue_pointer(int slot_id, int ep_index, struct urb *urb);

int f_hub_setportfeature(int hdev_num, int wValue, int wIndex);
int f_hub_clearportfeature(int hdev_num, int wValue, int wIndex);
int f_hub_sethubfeature(int hdev_num, int wValue);
int f_hub_config_subhub(int parent_hub_num, int hub_num, int port_num);
int f_hub_configep(int hdev_num, int rh_port_index);
int f_hub_configuredevice(int hub_num, int port_num, int dev_num
		, int transfer_type, int maxp, int bInterval, char is_config_ep, char is_stress, int stress_config);
int f_hub_reset_dev(struct usb_device *udev,int dev_num, int port_num, int speed);
int f_hub_configure_eth_device(int hub_num, int port_num, int dev_num);

int f_random_stop(int ep_1_num, int ep_2_num, int stop_count_1, int stop_count_2, int urb_dir_1, int urb_dir_2, int length);
int f_add_random_stop_ep_thread(struct xhci_hcd *xhci, int slot_id, int ep_index);
int f_add_random_ring_doorbell_thread(struct xhci_hcd *xhci, int slot_id, int ep_index);
int f_config_ep(char ep_num,int ep_dir,int transfer_type, int maxp,int bInterval, int burst, int mult, struct usb_device *udev,int config_xhci);
int f_deconfig_ep(char is_all, char ep_num,int ep_dir,struct usb_device *usbdev,int config_xhci);
int f_add_str_threads(int dev_num, int ep_num, int maxp, char isCompare, struct usb_device *usbdev, char isEP0);
int f_add_ixia_thread(struct xhci_hcd *xhci, int dev_num, struct ixia_dev *ix_dev);
int f_fill_urb(struct urb *urb,int ep_num,int data_length, int start_add,int dir, int iso_num_packets, int psize, struct usb_device *usbdev);
int f_fill_urb_with_buffer(struct urb *urb,int ep_num,int data_length,void *buffer,int start_add,int dir, int iso_num_packets, int psize
	, dma_addr_t dma_mapping, struct usb_device *usbdev);
int f_queue_urb(struct urb *urb,int wait, struct usb_device *dev);
int f_update_hub_device(struct usb_device *udev, int num_ports);

int f_loopback_loop(int ep_out, int ep_in, int data_length, int start_add, struct usb_device *usbdev);
int f_loopback_sg_loop(int ep_out, int ep_in, int data_length, int start_add, int sg_len, struct usb_device *usbdev);
int f_loopback_loop_gpd(int ep_out, int ep_in, int data_length, int start_add, int gpd_length, struct usb_device *usbdev);
int f_loopback_sg_loop_gpd(int ep_out, int ep_in, int data_length, int start_add, int sg_len, int gpd_length, struct usb_device *usbdev);

int mtk_xhci_handshake(struct xhci_hcd *xhci, void __iomem *ptr,
		      u32 mask, u32 done, int msec);

int f_data_out(int ep_out,int data_length,int start_add,int wait);
int f_data_in(int ep_in,int data_length, int start_add,int wait);

#if TEST_OTG
int otg_dev_A_host_thread(void *data);
int otg_dev_B_hnp(void * data);
int otg_dev_A_hnp(void * data);
int otg_dev_A_hnp_back(void * data);
int otg_dev_B_hnp_back(void *data);
int otg_dev_A_srp(void * data);
int otg_opt_uut_a(void *data);
int otg_opt_uut_b(void *data);
int otg_pet_uut_a(void *data);
int otg_pet_uut_b(void *data);
#endif

int f_test_lib_init(void);
int f_test_lib_cleanup(void);

struct urb *alloc_ctrl_urb(struct usb_ctrlrequest *dr, char *buffer, struct usb_device *udev);
int f_ctrlrequest(struct urb *urb, struct usb_device *udev);

int get_port_index(int port_id);
void print_speed(int speed);
int wait_event_on_timeout(volatile int *ptr, int done, int msecs);
int poll_event_on_timeout(volatile int *ptr, int done, int msecs);

void start_port_reenabled(int index, int speed);
int f_reenable_port(int index);

int f_enable_dev_note(void);

int f_add_rdn_len_str_threads(int dev_num, int ep_num, int maxp, char isCompare, struct usb_device *usbdev, char isEP0);
int f_add_random_access_reg_thread(struct xhci_hcd *xhci, int port_id, int port_rev, int power_required);
int f_power_reset_u1u2_counter(int u_num);
int f_power_get_u1u2_counter(int u_num);
int f_power_config_lpm(u32 slot_id, u32 hirdm, u32 L1_timeout, u32 rwe, u32 besl, u32 besld, u32 hle, u32 int_nak_active, u32 bulk_nyet_active);
int f_power_reset_L1_counter(int direct);
int f_power_get_L1_counter(int direct);
int f_port_set_pls(int port_id, int pls);
int f_ctrlrequest_nowait(struct urb *urb, struct usb_device *udev);
int wait_not_event_on_timeout(int *ptr, int value, int msecs);

/* just a wrapper function to avoid function in mtk-test.c call xhci-mtk.c directly */
static inline int f_chk_frmcnt_clk(struct usb_hcd * hcd)
{
	return chk_frmcnt_clk(hcd);
}


/* IP configuration */
#define RH_PORT_NUM 2
#define DEV_NUM 4
#define HUB_DEV_NUM 4

/* constant define */
#define MAX_DATA_LENGTH 65535

/* timeout */
#define ATTACH_TIMEOUT 2000
#define CMD_TIMEOUT    1000
#define TRANS_TIMEOUT  800
#define SYNC_DELAY     100

/* return code */
#define RET_SUCCESS 0
#define RET_FAIL -1

/* command stage */
#define CMD_RUNNING 0
#define CMD_DONE 1
#define CMD_FAIL 2

/* transfer stage */
#define TRANS_INPROGRESS 0
#define TRANS_DONE 1

/* loopback stage */
#define LOOPBACK_OUT  0
#define LOOPBACK_IN   1

/* USB spec constant */
/* EP descriptor */
#define EPADD_NUM(n) ((n)<<0)
#define EPADD_OUT 0
#define EPADD_IN  (1<<7)

#define EPATT_CTRL 0
#define EPATT_ISO  1
#define EPATT_BULK 2
#define EPATT_INT  3

#define MAX_DATA_LENGTH 65535

/* control request code */
#define REQ_GET_STATUS		0
#define REQ_SET_FEATURE 	3
#define REQ_CLEAR_FEATURE   1

/* hub request code */
#define HUB_FEATURE_PORT_POWER 		  8
#define HUB_FEATURE_PORT_RESET		  4
#define HUB_FEATURE_PORT_SUSPEND	2
#define HUB_FEATURE_C_PORT_CONNECTION 16
#define HUB_FEATURE_C_PORT_RESET	  20

#define HUB_FEATURE_PORT_LINK_STATE		5

/* global structure */
typedef enum
{
    DISCONNECTED = 0,
    CONNECTED,
    RESET,
    ENABLED
}XHCI_PORT_STATUS;

struct xhci_port
{
	int port_id;
	XHCI_PORT_STATUS port_status;
	int port_speed;
	int port_reenabled;
};

/* for stress test */
#define TOTAL_URB	30
#define URB_STATUS_IDLE	150
#define URB_STATUS_RX	151
#define GPD_LENGTH	(16*1024)
#define GPD_LENGTH_RDN	(10*1024)

/* global parameters */
#ifdef MTK_TEST_LIB
#define MTK_TEST_LIB_EXTERN
#else 
#define MTK_TEST_LIB_EXTERN  extern
#endif

MTK_TEST_LIB_EXTERN volatile char g_port_connect;
MTK_TEST_LIB_EXTERN volatile char g_port_reset;
MTK_TEST_LIB_EXTERN volatile int g_port_id;
MTK_TEST_LIB_EXTERN volatile int g_slot_id;
MTK_TEST_LIB_EXTERN struct urb *g_urb_tx, *g_urb_rx;
MTK_TEST_LIB_EXTERN volatile int g_speed;
MTK_TEST_LIB_EXTERN volatile int g_cmd_status;
MTK_TEST_LIB_EXTERN volatile char g_event_full;
MTK_TEST_LIB_EXTERN volatile char g_event_stall;
MTK_TEST_LIB_EXTERN volatile char g_got_event_full;
MTK_TEST_LIB_EXTERN volatile int g_device_reconnect;
MTK_TEST_LIB_EXTERN struct usb_device *dev_list[DEV_NUM];
MTK_TEST_LIB_EXTERN struct usb_device *hdev_list[HUB_DEV_NUM];
MTK_TEST_LIB_EXTERN struct usb_hcd *my_hcd;
MTK_TEST_LIB_EXTERN struct xhci_port *rh_port[RH_PORT_NUM];
MTK_TEST_LIB_EXTERN volatile int g_stress_status;
MTK_TEST_LIB_EXTERN struct ixia_dev *ix_dev_list[4];
MTK_TEST_LIB_EXTERN volatile char g_stopped;
MTK_TEST_LIB_EXTERN volatile char g_correct;
MTK_TEST_LIB_EXTERN volatile int g_dev_notification;	
MTK_TEST_LIB_EXTERN volatile long g_dev_not_value;
MTK_TEST_LIB_EXTERN volatile long g_intr_handled;
MTK_TEST_LIB_EXTERN volatile int g_mfindex_event;
MTK_TEST_LIB_EXTERN volatile char g_port_occ;
MTK_TEST_LIB_EXTERN volatile char g_is_bei;
MTK_TEST_LIB_EXTERN volatile char g_td_to_noop;
MTK_TEST_LIB_EXTERN volatile char g_iso_frame;
MTK_TEST_LIB_EXTERN volatile char g_test_random_stop_ep;
MTK_TEST_LIB_EXTERN volatile char g_stopping_ep;
MTK_TEST_LIB_EXTERN volatile char g_port_resume;
MTK_TEST_LIB_EXTERN volatile int g_cmd_ring_pointer1;
MTK_TEST_LIB_EXTERN volatile int g_cmd_ring_pointer2;
MTK_TEST_LIB_EXTERN volatile char g_idt_transfer;
MTK_TEST_LIB_EXTERN volatile int g_port_plc;
MTK_TEST_LIB_EXTERN volatile char g_power_down_allowed;
MTK_TEST_LIB_EXTERN volatile char g_hs_block_reset;
MTK_TEST_LIB_EXTERN volatile char g_concurrent_resume;
#if TEST_OTG
MTK_TEST_LIB_EXTERN volatile int g_otg_test;
MTK_TEST_LIB_EXTERN volatile int g_otg_dev_B;
MTK_TEST_LIB_EXTERN volatile int g_otg_hnp_become_host;
MTK_TEST_LIB_EXTERN volatile int g_otg_hnp_become_dev;
MTK_TEST_LIB_EXTERN volatile int g_otg_srp_pend;
MTK_TEST_LIB_EXTERN volatile int g_otg_pet_status;
MTK_TEST_LIB_EXTERN volatile int g_otg_csc;
MTK_TEST_LIB_EXTERN volatile int g_otg_iddig;
MTK_TEST_LIB_EXTERN volatile int g_otg_wait_con;
MTK_TEST_LIB_EXTERN volatile int g_otg_dev_conf_len;
MTK_TEST_LIB_EXTERN volatile int g_otg_unsupported_dev;
MTK_TEST_LIB_EXTERN volatile int g_otg_slot_enabled;
MTK_TEST_LIB_EXTERN volatile int g_otg_iddig_toggled;
typedef enum
{
    OTG_DISCONNECTED = 0,
    OTG_POLLING_STATUS,
    OTG_SET_NHP,
    OTG_HNP_INIT,
    OTG_HNP_SUSPEND,
    OTG_HNP_DEV,
    OTG_HNP_DISCONNECTED,
    OTG_DEV	
    
}PET_STATUS;
#endif
// Billionton Definition
#define AX_CMD_SET_SW_MII               0x06
#define AX_CMD_READ_MII_REG             0x07
#define AX_CMD_WRITE_MII_REG            0x08
#define AX_CMD_READ_MII_OPERATION_MODE      0x09
#define AX_CMD_SET_HW_MII               0x0a
#define AX_CMD_READ_EEPROM              0x0b
#define AX_CMD_WRITE_EEPROM             0x0c
#define AX_CMD_READ_RX_CONTROL_REG             0x0f
#define AX_CMD_WRITE_RX_CTL             0x10
#define AX_CMD_READ_IPG012              0x11
#define AX_CMD_WRITE_IPG0               0x12
#define AX_CMD_WRITE_IPG1               0x13
#define AX_CMD_WRITE_IPG2               0x14
#define AX_CMD_READ_MULTIFILTER_ARRAY           0x15
#define AX_CMD_WRITE_MULTI_FILTER       0x16
#define AX_CMD_READ_NODE_ID             0x17
#define AX_CMD_READ_PHY_ID                  0x19
#define AX_CMD_READ_MEDIUM_MODE    0x1a
#define AX_CMD_WRITE_MEDIUM_MODE        0x1b
#define AX_CMD_READ_MONITOR_MODE        0x1c
#define AX_CMD_WRITE_MONITOR_MODE       0x1d
#define AX_CMD_READ_GPIOS                   0x1e
#define AX_CMD_WRITE_GPIOS                  0x1f

#define AX_CMD_GUSBKR3_BREQ         0x05
#define AX_CMD_GUSBKR3_12e          0x12e
#define AX_CMD_GUSBKR3_120          0x120
#define AX_CMD_GUSBKR3_126          0x126
#define AX_CMD_GUSBKR3_134          0x134
#define AX_CMD_GUSBKR3_12f          0x12f
#define AX_CMD_GUSBKR3_130          0x130
#define AX_CMD_GUSBKR3_137          0x137
#define AX_CMD_GUSBKR3_02           0x02
#define AX_CMD_GUSBKR3_13e          0x13e

/*
* USB directions
*/
#define MUSB_DIR_OUT			0
#define MUSB_DIR_IN			0x80

/*
* USB request types
*/
#define MUSB_TYPE_MASK			(0x03 << 5)
#define MUSB_TYPE_STANDARD		(0x00 << 5)
#define MUSB_TYPE_CLASS			(0x01 << 5)
#define MUSB_TYPE_VENDOR		(0x02 << 5)
#define MUSB_TYPE_RESERVED		(0x03 << 5)

/*
* USB recipients
*/
#define MUSB_RECIP_MASK			0x1f
#define MUSB_RECIP_DEVICE		0x00
#define MUSB_RECIP_INTERFACE    0x01
#define MUSB_RECIP_ENDPOINT		0x02
#define MUSB_RECIP_OTHER		0x03

/*
* Standard requests
*/
#define MUSB_REQ_GET_STATUS		           0x00
#define MUSB_REQ_CLEAR_FEATURE	           0x01
#define MUSB_REQ_SET_FEATURE		           0x03
#define MUSB_REQ_SET_ADDRESS		           0x05
#define MUSB_REQ_GET_DESCRIPTOR		    0x06
#define MUSB_REQ_SET_DESCRIPTOR		    0x07
#define MUSB_REQ_GET_CONFIGURATION	    0x08
#define MUSB_REQ_SET_CONFIGURATION	    0x09
#define MUSB_REQ_GET_INTERFACE		    0x0A
#define MUSB_REQ_SET_INTERFACE		    0x0B
#define MUSB_REQ_SYNCH_FRAME                  0x0C
#define VENDOR_CONTROL_NAKTIMEOUT_TX    0x20
#define VENDOR_CONTROL_NAKTIMEOUT_RX    0x21
#define VENDOR_CONTROL_DISPING                0x22
#define VENDOR_CONTROL_ERROR                   0x23
#define VENDOR_CONTROL_RXSTALL                0x24

/*
* Descriptor types
*/
#define MUSB_DT_DEVICE			0x01
#define MUSB_DT_CONFIG			0x02
#define MUSB_DT_STRING			0x03
#define MUSB_DT_INTERFACE		0x04
#define MUSB_DT_ENDPOINT		0x05
#define MUSB_DT_DEVICE_QUALIFIER	0x06
#define MUSB_DT_OTHER_SPEED		0X07
#define MUSB_DT_INTERFACE_POWER		0x08
#define MUSB_DT_OTG			0x09
#define MUSB_DT_HUB			0x29

struct MUSB_DeviceRequest
{
    unsigned char bmRequestType;
    unsigned char bRequest;
    unsigned short wValue;
    unsigned short wIndex;
    unsigned short wLength;
};


struct ethenumeration_t
{
    unsigned char* pDesciptor;
    struct MUSB_DeviceRequest sDevReq;
};

struct MUSB_ConfigurationDescriptor
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short wTotalLength;
    unsigned char bNumInterfaces;
    unsigned char bConfigurationValue;
    unsigned char iConfiguration;
    unsigned char bmAttributes;
    unsigned char bMaxPower;
};

struct MUSB_InterfaceDescriptor
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bInterfaceNumber;
    unsigned char bAlternateSetting;
    unsigned char bNumEndpoints;
    unsigned char bInterfaceClass;
    unsigned char bInterfaceSubClass;
    unsigned char bInterfaceProtocol;
    unsigned char iInterface;
};

struct MUSB_EndpointDescriptor
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned char bEndpointAddress;
    unsigned char bmAttributes;
    unsigned short wMaxPacketSize;
    unsigned char bInterval;
};

struct MUSB_DeviceDescriptor
{
    unsigned char bLength;
    unsigned char bDescriptorType;
    unsigned short bcdUSB;
    unsigned char bDeviceClass;
    unsigned char bDeviceSubClass;
    unsigned char bDeviceProtocol;
    unsigned char bMaxPacketSize0;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char iManufacturer;
    unsigned char iProduct;
    unsigned char iSerialNumber;
    unsigned char bNumConfigurations;
};

void f_Descriptors_DescriptorType(u8 value);
u8 f_Descriptors_USB_Info(struct usb_device_descriptor *devDesc);

#endif

