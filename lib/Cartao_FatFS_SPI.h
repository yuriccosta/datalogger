
#ifndef CARTAO_FATFS_SPI_H
#define CARTAO_FATFS_SPI_H

extern char filename[20];

#include <stdbool.h>
#include <pico/stdlib.h>

void run_setrtc(void);
void run_format(void);
void run_mount(void);
void run_unmount(void);
void run_getfree(void);
void run_ls(void);
void run_cat(void);
bool save_imu_data_to_csv(uint numero_amostra, int16_t accel_x, int16_t accel_y, int16_t accel_z, int16_t gyro_x, int16_t gyro_y, int16_t gyro_z);
void read_file(const char *filename);
void run_help(void);
void process_stdio(int cRxedChar);


#endif // CARTAO_FATFS_SPI_H
