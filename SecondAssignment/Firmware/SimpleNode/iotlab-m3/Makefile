# name of your application
APPLICATION = Henhouse_4

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3

# This has to be the absolute path to the RIOT base directory:
#RIOTBASE ?= $(CURDIR)/../../RIOT
RIOTBASE ?= /home/salvo/RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

#Features to get CPUID
FEATURES_REQUIRED = periph_cpuid

# Modules to include:
USEMODULE += xtimer
# To use IotLab-M3's three leds, since they are connected through GPIO
USEMODULE += periph_gpio


#	Shell modules (to remove, for debug purpose only)
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

#	Pressure/Temperature sensor driver
USEMODULE += lps331ap

# 	Accelerometer/Magnetometer sensor driver
USEMODULE += lsm303dlhc

#	Gyroscope sensor driver
USEMODULE += l3g4200d

#	Light sensor driver
USEMODULE += isl29020

#	Network module

# 	Include packages that pull up and auto-init the link layer.
#	NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

#	Specify the mandatory networking modules for IPv6
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo # For response to PING6 messages
#	Optimize network stack to for use with a single network interface
USEMODULE += gnrc_netif_single

#	Include MQTT-S client
USEMODULE += emcute

# Default channel for 802.15.4
DEFAULT_CHANNEL ?= 26

include $(RIOTBASE)/Makefile.include
