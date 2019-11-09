SET PATH=D:\keil5\ARM\ARMCC\Bin;D:\Anaconda3;D:\Anaconda3\Library\mingw-w64\bin;D:\Anaconda3\Library\usr\bin;D:\Anaconda3\Library\bin;D:\Anaconda3\Scripts;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Program Files\WIDCOMM\Bluetooth Software\;C:\Program Files\WIDCOMM\Bluetooth Software\syswow64;C:\Program Files\Git\cmd;D:\openocd\0.6.0\bin;C:\Windows\system32\config\systemprofile\.dnx\bin;C:\Program Files\Microsoft DNX\Dnvm\;C:\Program Files (x86)\Windows Kits\8.1\Windows Performance Toolkit\;C:\Program Files\Microsoft SQL Server\120\Tools\Binn\;C:\Program Files\Microsoft SQL Server\130\Tools\Binn\;D:\Python\python3.7.3\Scripts\;D:\Python\python3.7.3\;D:\Qt\Tools\QtCreator\bin;D:\openocd\0.6.0\bin;D:\Fiddler
SET CPU_TYPE=STM32F103C8
SET CPU_VENDOR=STMicroelectronics
SET UV2_TARGET=USART1-DEMO
SET CPU_CLOCK=0x00B71B00
"D:\keil5\ARM\ARMCC\Bin\ArmAsm" --Via ".\startup_stm32f10x_md._ia"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\main.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\stm32f10x_it.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\usart1.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\stm32f10x_gpio.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\stm32f10x_rcc.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\stm32f10x_usart.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\misc.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\core_cm3.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmCC" --Via ".\system_stm32f10x.__i"
"D:\keil5\ARM\ARMCC\Bin\ArmLink" --Via ".\STM32-DEMO.lnp"
"D:\keil5\ARM\ARMCC\Bin\fromelf.exe" ".\STM32-DEMO.axf" --i32combined --output ".\STM32-DEMO.hex"
