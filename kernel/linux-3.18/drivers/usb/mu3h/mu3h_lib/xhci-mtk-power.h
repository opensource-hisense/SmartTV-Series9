#ifndef _XHCI_MTK_POWER_H
#define _XHCI_MTK_POWER_H

#include <linux/usb.h>
#include "xhci.h"

void enableXhciAllPortPower(struct xhci_hcd *xhci);
void enableAllClockPower(struct device *dev);
void disableAllClockPower(struct device *dev);
void disablePortClockPower(struct device *dev, int port_index, int port_rev);
void enablePortClockPower(struct device *dev, int port_index, int port_rev);
void enableXhciPortPower(struct xhci_hcd *xhci, int port_id);


#endif
