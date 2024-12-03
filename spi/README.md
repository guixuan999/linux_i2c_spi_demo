# 引脚

![](引脚.png)

# 编译

```shell
gcc -o spi spi_flash_demo.c
```

# flashrom

`flashrom` 是一个开源工具，可以直接用于编程 SPI Flash。



安装：

```shell
sudo apt update
sudo apt install -y flashrom
```

检测spi flash：

```shell
sudo flashrom -p linux_spi:dev=/dev/spidev0.0 
```

输出中应该显示检测到的 Flash 芯片型号，例如：

Found Winbond flash chip "W25Q64.V" (8192 kB, SPI) on linux_spi.



读取spi flash：

```shell
sudo flashrom -p linux_spi:dev=/dev/spidev0.0 -r backup.bin
```

写入spi flash：

```shell
sudo flashrom -p linux_spi:dev=/dev/spidev0.0 -w firmware.bin
```

擦除：

```shell
sudo flashrom -p linux_spi:dev=/dev/spidev0.0 -E
```



# 参考

Raspberry Pi I2C_SPI Demo.mhtml
