# 编译
```shell
gcc -o i2c i2c_eeprom_demo.c
```
# i2cdetect
```shell
gx@rpi4b:~/sandbox/i2c $ i2cdetect -y 1
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --                         
```
以上说明外接i2c slave device地址为0x50
# i2ctransfer
```shell
sudo i2ctransfer -y 1 w2@0x50 0x3F 0xC0 r64
```
其中w2@0x50 0x3F 0xC0表示写两个字节到I2C地址0x50, 写入的0x3FC0是存储器的读写地址r64表示读取64个字节
```shell
sudo i2ctransfer -y 1 w3@0x50 0x00 0x10 0xAB
```
以上命令向地址0x0010写入一个字节0xAB

# 参考
树莓派上如何操作外接I2C_EEPROM.mhtml
