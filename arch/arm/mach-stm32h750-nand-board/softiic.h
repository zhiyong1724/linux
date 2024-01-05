#ifndef __SOFTIIC_H__
#define __SOFTIIC_H__
int softiic_start(struct device *device);
int softiic_stop(struct device *device);
int softiic_send_byte(struct device *device, uint8_t data);
uint8_t softiic_read_byte(struct device *device);
int softiic_read_ack(struct device *device);
int softiic_send_nack(struct device *device);
#endif