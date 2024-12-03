#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define I2C_BUS "/dev/i2c-1"
#define EEPROM_ADDR 0x50

// use ATMEL AT24C128 (i2c eeprom)
// AT24C128 is orgnized as 256 pages of 64-bytes each
#define EEPROM_PAGE_SIZE 64
#define EEPROM_PAGES 256

// mem_address 可从任意地址开始，但是当length足够大时，芯片内部写指针会从当前page的最后面绕回到当前page的最前面。（page是对齐EEPROM_PAGE_SIZE字节边界的）
void write_to_eeprom(int fd, uint16_t mem_address, uint8_t *data, size_t length) {
    uint8_t buffer[2 + length];  // 两字节地址 + 数据
    buffer[0] = (mem_address >> 8) & 0xFF;  // 高字节
    buffer[1] = mem_address & 0xFF;         // 低字节
    for (size_t i = 0; i < length; i++) {
        buffer[2 + i] = data[i];
    }

    if (write(fd, buffer, 2 + length) != 2 + length) {
        perror("写入失败");
    }
    usleep(10000);  // 写入后延迟
}

// mem_address 可从任意地址开始，但是当length足够大时，芯片内部读指针会从本存储器的最后面绕回到最前面。
void read_from_eeprom(int fd, uint16_t mem_address, uint8_t *buffer, size_t length) {
    uint8_t address[2];
    address[0] = (mem_address >> 8) & 0xFF;  // 高字节
    address[1] = mem_address & 0xFF;         // 低字节

    // 写地址
    if (write(fd, address, 2) != 2) {
        perror("地址写入失败");
        return;
    }

    usleep(10000);  // 地址写入后延迟

    // 读取数据
    if (read(fd, buffer, length) != length) {
        perror("读取失败");
        return;
    }
}

uint8_t buf_page_out[EEPROM_PAGE_SIZE];
uint8_t buf_page_in[EEPROM_PAGE_SIZE];

static uint8_t *HexString(uint8_t *pBuf, size_t size);

int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0) {
        perror("无法打开 I2C 总线");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, EEPROM_ADDR) < 0) {
        perror("无法与 EEPROM 通信");
        return 1;
    }

	uint8_t page = 0; // page 0-255
	
	uint8_t value = 0;
	for(int i = 0; i < EEPROM_PAGE_SIZE; i++) {
		*(buf_page_out + i)= value++;
	}
    write_to_eeprom(fd, page * EEPROM_PAGE_SIZE, buf_page_out, EEPROM_PAGE_SIZE);
	read_from_eeprom(fd, page * EEPROM_PAGE_SIZE, buf_page_in, EEPROM_PAGE_SIZE);
    printf("读取到page_%d：\n%s\n", page, HexString(buf_page_in, EEPROM_PAGE_SIZE));
	printf("读出与写入匹配：%s\n", memcmp(buf_page_out, buf_page_in, EEPROM_PAGE_SIZE) == 0 ? "YES" : "NO");
	
	page = EEPROM_PAGES - 1;
	for(int i = 0; i < EEPROM_PAGE_SIZE; i++) {
		*(buf_page_out + i)= value++;
	}
	write_to_eeprom(fd, page * EEPROM_PAGE_SIZE, buf_page_out, EEPROM_PAGE_SIZE);
	read_from_eeprom(fd, page * EEPROM_PAGE_SIZE, buf_page_in, EEPROM_PAGE_SIZE);
    printf("读取到page_%d：\n%s\n", page, HexString(buf_page_in, EEPROM_PAGE_SIZE));
	printf("读出与写入匹配：%s\n", memcmp(buf_page_out, buf_page_in, EEPROM_PAGE_SIZE) == 0 ? "YES" : "NO");

    close(fd);
    return 0;
}

static uint8_t HexStringBuf[EEPROM_PAGE_SIZE * 3];
static uint8_t *HexString(uint8_t *pBuf, size_t size)
{
    if (size == 0)
        return NULL;

    HexStringBuf[0] = 0;

    for (int i = 0; i < size; i++)
    {
        if (i == size - 1)
        {
            sprintf(HexStringBuf + strlen(HexStringBuf), "%02x", pBuf[i]);
        }
        else
        {
            sprintf(HexStringBuf + strlen(HexStringBuf), "%02x ", pBuf[i]);
        }
    }
    return HexStringBuf;
}