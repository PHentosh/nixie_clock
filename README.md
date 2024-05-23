# Nixie Clock

This project is based on esp32 microcontroller. 
It uses MCP23017 as I2C expander and 1 to 16 multiplexer to controll each Lamp.

## Dependencies

To build this project you will need `esp-idf` framework. You can find instructions how to install it on official espressif site

Official docs: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html <br>
Official repo: https://github.com/espressif/esp-idf

## Build

To compile and flash this project on your controller use the following `make` commands:

### Make generate
This command will generate cmake project in `build` directory

```
$> make generate
```


### Make app
This command will build the project

```
$> make app
```

### Make flash
Tis command can be used to flash your project on chip <br>
Good practice is to erace all flash before flashing your project to exclude any colisions.<br>
Options: 
- `ESPPORT` to specify port where your programer is conected
- `ESPBAUD` to specify baudrate
```
$> make flash ESPPORT=/dev/ttyUSB3 ESPBAUD=115200 
```


### Make monitor
Tis command can be used to read logs of your device chip <br>
Options: 
- `ESPPORT` to specify port where your programer is conected
- `ESPBAUD` to specify baudrate, by default sould be `115200`. If you want to use different baudrate please change corresponding option in `menuconfig` (look into esp-idf docs for additional info)

```
$> make monitor ESPPORT=/dev/ttyUSB3 ESPBAUD=115200 
```


### Make clean
Clean build files

```
$> make clean
```


### Make clean-all
Clean all generated files for build, including generated cmake project

```
$> make clean-all
```


## [Presentation](https://docs.google.com/presentation/d/1f5WE6e0m0K4JSjKqZYukn4jPIfkcWSu3lMK0vQX4MUs/edit?usp=sharing)
