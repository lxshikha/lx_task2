# lx_task2

Instructions to build the code:-

1. Clone this git repository on your machine
git clone https://github.com/lxshikha/lx_task2

2. Go into the directory lx_task2, this directory contains all the driver related files with Makefile to build the code.

3. Before starting the build, please ensure that this directory will also contain linux headers folder. Version of headers should match the kernel version of your development platform.  

4. It would be required to change the KDIR entry in the Makefile according to the headers used. Have a look inside Makefile.

5. Run the make command as given below to build the driver files and device tree overlay
    make lxaccelldriver

Above command will build driver and device tree overlay, it also copies the built device tree overlay blob to the directory /lib/firmware so that in further steps device tree overlay blob could be exported.

6. LIS3DH connection with Beaglebone
In this experiment LIS3DH is connected with Beaglebone on I2C1 

                 | LIS3DH Pin    | BBB Pin       |
                 | ------------- | ------------- |
                 | Vin           | P9_4          |
                 | SCL           | P9_17         |
                 | SDA           | P9_18         |
                 | CS            | P9_3          |
                 | INT           | P9_23         |
                 | SDO           | P9_1          |
                 | GND           | P9_2          | 
-Above connection provide below configuration:
- LIS3DH address on I2C bus = 0x18 (SDO = 0(gnd))
- Communication between beaglebona and LIS3DH is I2C (CS = high)
                     
7. After driver build and hardware connection:-
  - (i) Export device tree overlay blob :   
   echo BB-LX-ACCEL > /sys/devices/platform/bone_capemgr/slots
  - (ii) Insert the driver using insmod  
     insmod lxdriver.ko   
      
8. After above steps lxaccell device file will be created /dev/lxaccell
  Now cat command can be used to get accelleration data from LIS3DH using cat command:- cat /dev/lxaccell
 
 9.sample output is given below:  
 root@beaglebone:~/lx/accell_driver# cat /dev/lxaccell  
-76,-276,960  
-80,-272,964  
-92,-280,968  
-72,-276,956  
-80,-276,968  
-84,-268,960  
-80,-280,960  
-92,-280,964  
-80,-284,960  
-92,-284,964  
-80,-272,964  
-88,-276,964  

  
  
  
