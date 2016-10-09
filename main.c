#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "inc/hw_gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include <stdint.h>
#include <stdbool.h>
//*********************************************
#define GPIO_PA6_I2C1SCL        0x00001803
#define GPIO_PA7_I2C1SDA        0x00001C03
#define GPIO_PA0_U0RX           0x00000001
#define GPIO_PA1_U0TX           0x00000401
#define MPU6050_SMPLRT_DIV                  0x19
#define MPU6050_INT_PIN_CFG                 0x37
#define MPU6050_ACCEL_XOUT_H                0x3B
#define MPU6050_GYRO_XOUT_H                 0x43
#define MPU6050_PWR_MGMT_1                  0x6B
#define MPU6050_WHO_AM_I                    0x75
#define MPU6050_ADDRESS                     0x68
// Scale factor for +-2000deg/s and +-8g - see datasheet:
#define MPU6050_GYRO_SCALE_FACTOR_2000      16.4f
#define MPU6050_ACC_SCALE_FACTOR_8          4096.0f
//*********************************************
// Initialize UART
void UARTInit(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlDelay(2); // Insert a few cycles after enabling the peripheral to allow the clock to be fully activated
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //enable GPIO port for LED
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2); //enable pin for LED PF2
	UARTStdioConfig(0,115200,SysCtlClockGet());
}
// Initialize I2C
void I2CInit(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1); // Enable I2C1 peripheral
    SysCtlDelay(2); // Insert a few cycles after enabling the peripheral to allow the clock to be fully activated
    // Use alternate function
    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6); // Use pin with I2C SCL peripheral
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7); // Use pin with I2C peripheral
    I2CMasterInitExpClk(I2C1_BASE, SysCtlClockGet(), true); // Enable and set frequency to 400 kHz
    SysCtlDelay(2); // Insert a few cycles after enabling the I2C to allow the clock to be fully activated
}
//asdffghjjkklll
// Write data to I2C module function, upon one byte data
void write_mpu(uint8_t addr, uint8_t regAddr, uint8_t *data, uint8_t lenght) {
	I2CMasterSlaveAddrSet(I2C1_BASE, addr, false); // Set to write mode
	I2CMasterDataPut(I2C1_BASE, regAddr); // Place address into data register
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START); // Start condition I2C
	while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
	uint8_t i;
	for(i=0;i<lenght-1;i++) {
		I2CMasterDataPut(I2C1_BASE, data[i]); // Place data into data register
		I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_CONT); // Continute condition I2C
		while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
	}
	I2CMasterDataPut(I2C1_BASE, data[lenght-1]); //Place the final data into data register
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); // Stop condition I2C
	while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
}
// Write one byte data to I2C module
void single_write_mpu(uint8_t addr, uint8_t regAddr, uint8_t data) {
	write_mpu(addr,regAddr, &data, 1);
}
// Read data from I2C module function, upon one byte data
void read_mpu(uint8_t addr, uint8_t regAddr, uint8_t *data, uint8_t lenght) {
	I2CMasterSlaveAddrSet(I2C1_BASE, addr, false); // Set to write mode
	I2CMasterDataPut(I2C1_BASE, regAddr); // Place address into data register
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_SEND); // Send one byte data
	while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
	I2CMasterSlaveAddrSet(I2C1_BASE, addr, true); // Set to read mode
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START); // Start condition I2C
	while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
	data[0]=I2CMasterDataGet(I2C1_BASE); // Place data into register
	uint8_t i;
	for(i=1;i<lenght-1;i++) {
		I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT); // Continute condition I2C
		while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
		data[i]=I2CMasterDataGet(I2C1_BASE); // Place data into data register
	}
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH); //Stop condition I2C
	while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
	data[lenght-1]=I2CMasterDataGet(I2C1_BASE);// Place data into data register
}
// Read one byte data from I2C module
uint8_t single_read_mpu(uint8_t addr, uint8_t regAddr) {
	I2CMasterSlaveAddrSet(I2C1_BASE, addr, false); // Set to write mode
	I2CMasterDataPut(I2C1_BASE, regAddr); // Place address into data register
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_SEND); // Send one byte data
	while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
	I2CMasterSlaveAddrSet(I2C1_BASE, addr, true); // Set to read mode
	I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE); // Read one byte data
	while(I2CMasterBusy(I2C1_BASE)); // Wait until tranfer is done
	return I2CMasterDataGet(I2C1_BASE); // Place data into register
}
// Initialize MPU6050
void MPU6050Init(void) {
	uint8_t I2CBuffer[5]; // Buffer for initialize MPU6050
	I2CBuffer[0]=single_read_mpu(MPU6050_ADDRESS, MPU6050_WHO_AM_I);
	single_write_mpu(MPU6050_ADDRESS, MPU6050_PWR_MGMT_1, (1<<7)); // Reset all data registers to defaut values
	SysCtlDelay(SysCtlClockGet()/100); // Delay a bit
	while(single_read_mpu(MPU6050_ADDRESS,MPU6050_PWR_MGMT_1)&(1<<7)) {
		// Wait the bit clear
	}
	SysCtlDelay(SysCtlClockGet()/100);
	single_write_mpu(MPU6050_ADDRESS, MPU6050_PWR_MGMT_1, (1 << 3) | (1 << 0));
	// Disable sleep mode, disable temperature sensor and use PLL as clock reference
	I2CBuffer[0] = 0; // Set the sample rate to 1kHz - 1kHz/(1+0) = 1kHz
	I2CBuffer[1] = 0x03; // Disable FSYNC and set 41 Hz Gyro filtering, 1 KHz sampling
	I2CBuffer[2] = 3 << 3; // Set Gyro Full Scale Range to +-2000deg/s
	I2CBuffer[3] = 2 << 3; // Set Accelerometer Full Scale Range to +-8g
	I2CBuffer[4] = 0x03; // 41 Hz Acc filtering
	write_mpu(MPU6050_ADDRESS, MPU6050_SMPLRT_DIV, I2CBuffer, 5); // Write to all five registers at once
	// Enable Raw Data Ready Interrupt on INT pin
	I2CBuffer[0] = (1 << 5) | (1 << 4); // Enable LATCH_INT_EN and INT_ANYRD_2CLEAR
	// When this bit is equal to 1, the INT pin is held high until the interrupt is cleared
	// When this bit is equal to 1, interrupt status is cleared if any read operation is performed
	I2CBuffer[1] = (1 << 0);            // Enable RAW_RDY_EN - When set to 1, Enable Raw Sensor Data
										//Ready interrupt to propagate to interrupt pin
	write_mpu(MPU6050_ADDRESS, MPU6050_INT_PIN_CFG, I2CBuffer, 2); // Write to both registers at once
}
//*********************************************
void main() {
	// Configure clock 50 MHz
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |SYSCTL_XTAL_16MHZ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	UARTInit();
	I2CInit();
	MPU6050Init();
	uint8_t buffer[14];
	int16_t accelX;
	UARTprintf("Bat dau doc gia toc he-----\n");
	while(1) {
		read_mpu(MPU6050_ADDRESS, MPU6050_ACCEL_XOUT_H, buffer, 14); // Note that we can't write directly
											//into MPU6050_t, because of endian conflict. So it has to be done manually
		accelX = (buffer[0]<<8)|buffer[1];
		SysCtlDelay(SysCtlClockGet()/3); //delay a bit
		UARTprintf("Gia toc theo truc X: %d\n",accelX);
	}
}
// adfdsfdgfgj