# name of your application
APPLICATION = Henhouse

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-f401re

# This has to be the absolute path to the RIOT base directory:
#RIOTBASE ?= $(CURDIR)/../../RIOT
RIOTBASE ?= /home/salvo/RIOT

# Default to using ethos for providing the uplink when not on native
UPLINK ?= ethos

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:
	#Ethos & UHCPC modules
USEMODULE += stdio_ethos
USEMODULE += gnrc_uhcpc

	# Include packages that pull up and auto-init the link layer.
	# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

	# Specify the mandatory networking modules for IPv6
USEMODULE += gnrc_ipv6_default

	# Optimize network stack to for use with a single network interface
USEMODULE += gnrc_netif_single

	# Include MQTT-S client
USEMODULE += emcute

USEMODULE += xtimer
USEMODULE += periph_gpio
USEMODULE += analog_util

	# Shell modules
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

FEATURES_REQUIRED += periph_adc


# Allow for env-var-based override of the nodes name (EMCUTE_ID)
ifneq (,$(EMCUTE_ID))
  CFLAGS += -DEMCUTE_ID=\"$(EMCUTE_ID)\"
endif

# include DHCP client
USE_DHCPV6 ?= 0

# Ethos/native TAP interface and UHCP prefix can be configured from make command
TAP ?= tap0
IPV6_PREFIX ?= fe80:2::/64
#IPV6_PREFIX ?= fec0:affe::/64

# The Broker address, port and the default MQTT topic to subscribe.
SERVER_ADDR = fe80::1
#SERVER_ADDR = fec0:affe::1
SERVER_PORT = 1885
MQTT_TOPIC = tft

CFLAGS += -DSERVER_ADDR='"$(SERVER_ADDR)"'
CFLAGS += -DSERVER_PORT=$(SERVER_PORT)
CFLAGS += -DMQTT_TOPIC='"$(MQTT_TOPIC)"'

ETHOS_BAUDRATE ?= 115200
include $(CURDIR)/Makefile.ethos.conf

include $(RIOTBASE)/Makefile.include

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS $(MAKE) -C $(RIOTTOOLS)
