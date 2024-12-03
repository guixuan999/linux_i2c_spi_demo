#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// SPI 配置参数
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_MODE SPI_MODE_0
#define SPI_BITS_PER_WORD 8
#define SPI_SPEED 500000  // 500 kHz

#define FLASH_SIZE (2*1024*1024) // 2MB
#define FLASH_PAGE_SIZE 256
#define FLASH_PAGES (FLASH_SIZE / FLASH_PAGE_SIZE)
#define FLASH_PAGES_PER_SECTOR 16
#define FLASH_SECTOR_SIZE (FLASH_PAGE_SIZE * FLASH_PAGES_PER_SECTOR)
#define FLASH_SECTORS (FLASH_PAGES / FLASH_PAGES_PER_SECTOR)

static uint8_t *HexString(uint8_t *pBuf, size_t size);

int spi_init(const char *device) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SPI device");
        exit(1);
    }

    uint8_t mode = SPI_MODE;
    uint8_t bits_per_word = SPI_BITS_PER_WORD;          // 每字传输位数
    uint32_t speed = SPI_SPEED;            // 速度：500 kHz

    // 设置 SPI 模式
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("Failed to set SPI mode");
        close(fd);
        exit(1);
    }

    // 设置每字传输位数
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0) {
        perror("Failed to set bits per word");
        close(fd);
        exit(1);
    }

    // 设置速度
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Failed to set max speed");
        close(fd);
        exit(1);
    }

    return fd;
}


void read_device_id(int spi_fd) {
    uint8_t tx[] = {0x9F, 0, 0, 0};  // 指令 + 占位字节
    uint8_t rx[4] = {0};

    // 发送指令并接收响应
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = sizeof(tx),
    //    .speed_hz = SPI_SPEED,
    //    .bits_per_word = SPI_BITS_PER_WORD,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("Failed to transfer SPI message");
        exit(1);
    }

    printf("Device ID: %02X %02X %02X\n", rx[1], rx[2], rx[3]);
}

void read_data(int spi_fd, uint32_t address, uint8_t *buffer, size_t length) {
    uint8_t cmd[4] = {
        0x03,                     // 标准读取命令
        (address >> 16) & 0xFF,   // 地址高字节
        (address >> 8) & 0xFF,    // 地址中字节
        address & 0xFF            // 地址低字节
    };

    struct spi_ioc_transfer tr[2] = {
        {
            .tx_buf = (unsigned long)cmd,
            .len = sizeof(cmd),
        //    .speed_hz = SPI_SPEED,
        //    .bits_per_word = SPI_BITS_PER_WORD,
        },
        {
            .rx_buf = (unsigned long)buffer,
            .len = length,
        //    .speed_hz = SPI_SPEED,
        //    .bits_per_word = SPI_BITS_PER_WORD,
        }
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(2), tr) < 0) {
        perror("Failed to read data");
        exit(1);
    }
}

void wait_for_ready(int spi_fd);
// this won't work!!! Please use read_data() 
// see to know why
void read_data_x(int spi_fd, uint32_t address, uint8_t *buffer, size_t length) {
    uint8_t cmd[4] = {
        0x03,                     // 标准读取命令
        (address >> 16) & 0xFF,   // 地址高字节
        (address >> 8) & 0xFF,    // 地址中字节
        address & 0xFF            // 地址低字节
    };

    write(spi_fd, cmd, 4 + length);
    wait_for_ready(spi_fd);
    read(spi_fd, buffer, length);
}

void write_enable(int spi_fd) {
    uint8_t cmd = 0x06;  // 写使能指令

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)&cmd,
        .len = sizeof(cmd),
    //    .speed_hz = SPI_SPEED,
    //    .bits_per_word = SPI_BITS_PER_WORD,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("Failed to enable write");
        exit(1);
    }
}

int read_status_register(int spi_fd) {
    uint8_t cmd = 0x05;  // 读取状态寄存器命令
    uint8_t status;

    struct spi_ioc_transfer tr[2] = {
        {
            .tx_buf = (unsigned long)&cmd,
            .len = 1,
        //    .speed_hz = SPI_SPEED,
        //    .bits_per_word = SPI_BITS_PER_WORD,
        },
        {
            .rx_buf = (unsigned long)&status,
            .len = 1,
        //    .speed_hz = SPI_SPEED,
        //    .bits_per_word = SPI_BITS_PER_WORD,
        }
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(2), tr) < 0) {
        perror("Failed to read status register");
        exit(1);
    }

    return status;
}

void wait_for_ready(int spi_fd) {
    while (read_status_register(spi_fd) & 0x01) {  // 检查忙标志
        usleep(1000);  // 等待 1ms
    }
}

// both write_data and write_data_x work! 
void write_data(int spi_fd, uint32_t address, const uint8_t *data, size_t length) {
    write_enable(spi_fd);

    uint8_t *cmd = malloc(4 + length);
    cmd[0] = 0x02;  // 页编程指令
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;
    memcpy(cmd + 4, data, length);

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)cmd,
        .len = 4 + length,
    //    .speed_hz = SPI_SPEED,
    //    .bits_per_word = SPI_BITS_PER_WORD,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("Failed to write data");
        exit(1);
    }

    free(cmd);
    wait_for_ready(spi_fd);  // 等待写完成
}

// both write_data and write_data_x work!
void write_data_x(int spi_fd, uint32_t address, const uint8_t *data, size_t length) {
    write_enable(spi_fd);

    uint8_t *cmd = malloc(4 + length);
    cmd[0] = 0x02;  // 页编程指令
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;
    memcpy(cmd + 4, data, length);

    write(spi_fd, cmd, 4 + length);

    free(cmd);
    wait_for_ready(spi_fd);  // 等待写完成
}

void erase_sector(int spi_fd, uint32_t address) {
    // Step 1: 写使能
    uint8_t write_enable_cmd = 0x06;  // 写使能命令
    if (write(spi_fd, &write_enable_cmd, 1) != 1) {
        perror("Failed to send Write Enable command");
        exit(1);
    }

    // Step 2: 发送擦除命令和地址
    uint8_t cmd[4] = {
        0x20,                      // 扇区擦除命令
        (address >> 16) & 0xFF,    // 地址高字节
        (address >> 8) & 0xFF,     // 地址中字节
        address & 0xFF             // 地址低字节
    };

    if (write(spi_fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        perror("Failed to send Erase Sector command");
        exit(1);
    }

    // Step 3: 等待擦除完成
    wait_for_ready(spi_fd);
}

void erase_chip(int spi_fd) {
    // Step 1: 写使能
    uint8_t write_enable_cmd = 0x06;  // 写使能命令
    if (write(spi_fd, &write_enable_cmd, 1) != 1) {
        perror("Failed to send Write Enable command");
        exit(1);
    }

    // Step 2: 发送芯片擦除命令
    uint8_t cmd = 0xC7;  // 芯片擦除命令
    if (write(spi_fd, &cmd, 1) != 1) {
        perror("Failed to send Chip Erase command");
        exit(1);
    }

    // Step 3: 等待擦除完成
    wait_for_ready(spi_fd);
    printf("Chip erased successfully.\n");
}


int main() {
    int spi_fd = spi_init(SPI_DEVICE);

    // 读取设备 ID
    read_device_id(spi_fd);

    // erase_chip(spi_fd);

    // 写入数据
    uint8_t data_to_write[FLASH_PAGE_SIZE];
    for(int i = 0; i < FLASH_PAGE_SIZE; i++) {
        *(data_to_write + i) = i;
    }

    int sector = 0;
    int page = 0;
    erase_sector(spi_fd, sector * FLASH_SECTOR_SIZE);
    for(int i = 0; i < FLASH_PAGES_PER_SECTOR; i++) {
        page = sector * FLASH_PAGES_PER_SECTOR + i;
        write_data(spi_fd, page * FLASH_PAGE_SIZE, data_to_write, sizeof(data_to_write));
    }
    
    sector = FLASH_SECTORS - 1; // the last sector
    erase_sector(spi_fd, sector * FLASH_SECTOR_SIZE);
    for(int i = 0; i < FLASH_PAGES_PER_SECTOR; i++) {
        page = sector * FLASH_PAGES_PER_SECTOR + i;
        write_data(spi_fd, page * FLASH_PAGE_SIZE, data_to_write, sizeof(data_to_write));
    }

    sector = 3;
    erase_sector(spi_fd, sector * FLASH_SECTOR_SIZE);
    for(int i = 0; i < FLASH_PAGES_PER_SECTOR; i++) {
        page = sector * FLASH_PAGES_PER_SECTOR + i;
        write_data(spi_fd, page * FLASH_PAGE_SIZE + 7, data_to_write, 17);
    }
    
    // 读取数据
    uint8_t buffer[FLASH_PAGE_SIZE];
    sector = 0;
    page = sector * FLASH_PAGES_PER_SECTOR;
    read_data(spi_fd, page * FLASH_PAGE_SIZE, buffer, sizeof(buffer));
    printf("Read Data @0x%06x: \n%s\n", page * FLASH_PAGE_SIZE, HexString(buffer, sizeof(buffer)));
    
    sector = 3;
    page = sector * FLASH_PAGES_PER_SECTOR + 1;
    read_data(spi_fd, page * FLASH_PAGE_SIZE, buffer, sizeof(buffer));
    printf("Read Data @0x%06x: \n%s\n",  page * FLASH_PAGE_SIZE, HexString(buffer, sizeof(buffer)));
    

    sector = FLASH_SECTORS - 1;
    page = sector * FLASH_PAGES_PER_SECTOR;
    read_data(spi_fd, page * FLASH_PAGE_SIZE + 13, buffer, 15);
    printf("Read Data @0x%06x: \n%s\n",  page * FLASH_PAGE_SIZE + 13, HexString(buffer, 15));
    
    close(spi_fd);
    return 0;
}

static uint8_t HexStringBuf[FLASH_PAGE_SIZE * 3];
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