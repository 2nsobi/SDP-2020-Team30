conf_file = '''

##### hostapd configuration file ##############################################
# Empty lines and lines starting with # are ignored


# The own IP address of the access point (used as NAS-IP-Address)
#own_ip_addr=127.0.0.1
own_ip_addr={}

# AP netdevice name (without 'ap' postfix, i.e., wlan0 uses wlan0ap for
# management frames with the Host AP driver); wlan0 with many nl80211 drivers
# Note: This attribute can be overridden by the values supplied with the '-i'
# command line parameter.
interface=wlp1s0

# In case of atheros and nl80211 driver interfaces, an additional
# configuration parameter, bridge, may be used to notify hostapd if the
# interface is included in a bridge. This parameter is not used with Host AP
# driver. If the bridge parameter is not set, the drivers will automatically
# figure out the bridge interface (assuming sysfs is enabled and mounted to
# /sys) and this parameter may not be needed.
#
# For nl80211, this parameter can be used to request the AP interface to be
# added to the bridge automatically (brctl may refuse to do this before hostapd
# has been started to change the interface mode). If needed, the bridge
# interface is also created.
bridge=br0

# Driver interface type (hostap/wired/none/nl80211/bsd);
# default: hostap). nl80211 is used with all Linux mac80211 drivers.
# Use driver=none if building hostapd as a standalone RADIUS server that does
# not control any wireless/wired driver.
# driver=hostap

# Driver interface parameters (mainly for development testing use)
# driver_params=<params>

# hostapd event logger configuration
#
# Two output method: syslog and stdout (only usable if not forking to
# background).
#
# Module bitfield (ORed bitfield of modules that will be logged; -1 = all
# modules):
# bit 0 (1) = IEEE 802.11
# bit 1 (2) = IEEE 802.1X
# bit 2 (4) = RADIUS
# bit 3 (8) = WPA
# bit 4 (16) = driver interface
# bit 6 (64) = MLME
#
# Levels (minimum value for logged events):
#  0 = verbose debugging
#  1 = debugging
#  2 = informational messages
#  3 = notification
#  4 = warning
#
logger_syslog=-1
logger_syslog_level=2
logger_stdout=-1
logger_stdout_level=2

# Interface for separate control program. If this is specified, hostapd
# will create this directory and a UNIX domain socket for listening to requests
# from external programs (CLI/GUI, etc.) for status information and
# configuration. The socket file will be named based on the interface name, so
# multiple hostapd processes/interfaces can be run at the same time if more
# than one interface is used.
# /var/run/hostapd is the recommended directory for sockets and by default,
# hostapd_cli will use it when trying to connect with hostapd.
ctrl_interface=/var/run/hostapd

# Access control for the control interface can be configured by setting the
# directory to allow only members of a group to use sockets. This way, it is
# possible to run hostapd as root (since it needs to change network
# configuration and open raw sockets) and still allow GUI/CLI components to be
# run as non-root users. However, since the control interface can be used to
# change the network configuration, this access needs to be protected in many
# cases. By default, hostapd is configured to use gid 0 (root). If you
# want to allow non-root users to use the control interface, add a new group
# and change this value to match with that group. Add users that should have
# control interface access to this group.
#
# This variable can be a group name or gid.
#ctrl_interface_group=wheel
ctrl_interface_group=0


##### IEEE 802.11 related configuration #######################################

# SSID to be used in IEEE 802.11 management frames
ssid={}
# Alternative formats for configuring SSID
# (double quoted string, hexdump, printf-escaped string)
#ssid2="test"
#ssid2=74657374
#ssid2=P"hellothere"

# UTF-8 SSID: Whether the SSID is to be interpreted using UTF-8 encoding
#utf8_ssid=1

# Country code (ISO/IEC 3166-1). Used to set regulatory domain.
# Set as needed to indicate country in which device is operating.
# This can limit available channels and transmit power.
# These two octets are used as the first two octets of the Country String
# (dot11CountryString)
#country_code=US

# The third octet of the Country String (dot11CountryString)
# This parameter is used to set the third octet of the country string.
#
# All environments of the current frequency band and country (default)
#country3=0x20
# Outdoor environment only
#country3=0x4f
# Indoor environment only
#country3=0x49
# Noncountry entity (country_code=XX)
#country3=0x58
# IEEE 802.11 standard Annex E table indication: 0x01 .. 0x1f
# Annex E, Table E-4 (Global operating classes)
#country3=0x04

# Enable IEEE 802.11d. This advertises the country_code and the set of allowed
# channels and transmit power levels based on the regulatory limits. The
# country_code setting must be configured with the correct country for
# IEEE 802.11d functions.
# (default: 0 = disabled)
#ieee80211d=1

# Enable IEEE 802.11h. This enables radar detection and DFS support if
# available. DFS support is required on outdoor 5 GHz channels in most countries
# of the world. This can be used only with ieee80211d=1.
# (default: 0 = disabled)
#ieee80211h=1

# Add Power Constraint element to Beacon and Probe Response frames
# This config option adds Power Constraint element when applicable and Country
# element is added. Power Constraint element is required by Transmit Power
# Control. This can be used only with ieee80211d=1.
# Valid values are 0..255.
#local_pwr_constraint=3

# Set Spectrum Management subfield in the Capability Information field.
# This config option forces the Spectrum Management bit to be set. When this
# option is not set, the value of the Spectrum Management bit depends on whether
# DFS or TPC is required by regulatory authorities. This can be used only with
# ieee80211d=1 and local_pwr_constraint configured.
#spectrum_mgmt_required=1

# Operation mode (a = IEEE 802.11a (5 GHz), b = IEEE 802.11b (2.4 GHz),
# g = IEEE 802.11g (2.4 GHz), ad = IEEE 802.11ad (60 GHz); a/g options are used
# with IEEE 802.11n (HT), too, to specify band). For IEEE 802.11ac (VHT), this
# needs to be set to hw_mode=a. For IEEE 802.11ax (HE) on 6 GHz this needs
# to be set to hw_mode=a. When using ACS (see channel parameter), a
# special value "any" can be used to indicate that any support band can be used.
# This special case is currently supported only with drivers with which
# offloaded ACS is used.
# Default: IEEE 802.11b
hw_mode=g

# Channel number (IEEE 802.11)
# (default: 0, i.e., not set)
# Please note that some drivers do not use this value from hostapd and the
# channel will need to be configured separately with iwconfig.
#
# If CONFIG_ACS build option is enabled, the channel can be selected
# automatically at run time by setting channel=acs_survey or channel=0, both of
# which will enable the ACS survey based algorithm.
channel={}

# Global operating class (IEEE 802.11, Annex E, Table E-4)
# This option allows hostapd to specify the operating class of the channel
# configured with the channel parameter. channel and op_class together can
# uniquely identify channels across different bands, including the 6 GHz band.
#op_class=131

# ACS tuning - Automatic Channel Selection
# See: http://wireless.kernel.org/en/users/Documentation/acs
#
# You can customize the ACS survey algorithm with following variables:
#
# acs_num_scans requirement is 1..100 - number of scans to be performed that
# are used to trigger survey data gathering of an underlying device driver.
# Scans are passive and typically take a little over 100ms (depending on the
# driver) on each available channel for given hw_mode. Increasing this value
# means sacrificing startup time and gathering more data wrt channel
# interference that may help choosing a better channel. This can also help fine
# tune the ACS scan time in case a driver has different scan dwell times.
#
# acs_chan_bias is a space-separated list of <channel>:<bias> pairs. It can be
# used to increase (or decrease) the likelihood of a specific channel to be
# selected by the ACS algorithm. The total interference factor for each channel
# gets multiplied by the specified bias value before finding the channel with
# the lowest value. In other words, values between 0.0 and 1.0 can be used to
# make a channel more likely to be picked while values larger than 1.0 make the
# specified channel less likely to be picked. This can be used, e.g., to prefer
# the commonly used 2.4 GHz band channels 1, 6, and 11 (which is the default
# behavior on 2.4 GHz band if no acs_chan_bias parameter is specified).
#
# Defaults:
#acs_num_scans=5
#acs_chan_bias=1:0.8 6:0.8 11:0.8

# Channel list restriction. This option allows hostapd to select one of the
# provided channels when a channel should be automatically selected.
# Channel list can be provided as range using hyphen ('-') or individual
# channels can be specified by space (' ') separated values
# Default: all channels allowed in selected hw_mode
#chanlist=100 104 108 112 116
#chanlist=1 6 11-13

# Frequency list restriction. This option allows hostapd to select one of the
# provided frequencies when a frequency should be automatically selected.
# Frequency list can be provided as range using hyphen ('-') or individual
# frequencies can be specified by comma (',') separated values
# Default: all frequencies allowed in selected hw_mode
#freqlist=2437,5955,5975
#freqlist=2437,5985-6105

# Exclude DFS channels from ACS
# This option can be used to exclude all DFS channels from the ACS channel list
# in cases where the driver supports DFS channels.
#acs_exclude_dfs=1

# Include only preferred scan channels from 6 GHz band for ACS
# This option can be used to include only preferred scan channels in the 6 GHz
# band. This can be useful in particular for devices that operate only a 6 GHz
# BSS without a collocated 2.4/5 GHz BSS.
# Default behavior is to include all PSC and non-PSC channels.
#acs_exclude_6ghz_non_psc=1

# Beacon interval in kus (1.024 ms) (default: 100; range 15..65535)
beacon_int={}

# DTIM (delivery traffic information message) period (range 1..255):
# number of beacons between DTIMs (1 = every beacon includes DTIM element)
# (default: 2)
dtim_period=2

# Maximum number of stations allowed in station table. New stations will be
# rejected after the station table is full. IEEE 802.11 has a limit of 2007
# different association IDs, so this number should not be larger than that.
# (default: 2007)
max_num_sta=255

# RTS/CTS threshold; -1 = disabled (default); range -1..65535
# If this field is not included in hostapd.conf, hostapd will not control
# RTS threshold and 'iwconfig wlan# rts <val>' can be used to set it.
rts_threshold=-1

# Fragmentation threshold; -1 = disabled (default); range -1, 256..2346
# If this field is not included in hostapd.conf, hostapd will not control
# fragmentation threshold and 'iwconfig wlan# frag <val>' can be used to set
# it.
fragm_threshold=-1

# Rate configuration
# Default is to enable all rates supported by the hardware. This configuration
# item allows this list be filtered so that only the listed rates will be left
# in the list. If the list is empty, all rates are used. This list can have
# entries that are not in the list of rates the hardware supports (such entries
# are ignored). The entries in this list are in 100 kbps, i.e., 11 Mbps = 110.
# If this item is present, at least one rate have to be matching with the rates
# hardware supports.
# default: use the most common supported rate setting for the selected
# hw_mode (i.e., this line can be removed from configuration file in most
# cases)
#supported_rates=10 20 55 110 60 90 120 180 240 360 480 540

# Basic rate set configuration
# List of rates (in 100 kbps) that are included in the basic rate set.
# If this item is not included, usually reasonable default set is used.
#basic_rates=10 20
#basic_rates=10 20 55 110
#basic_rates=60 120 240

# Beacon frame TX rate configuration
# This sets the TX rate that is used to transmit Beacon frames. If this item is
# not included, the driver default rate (likely lowest rate) is used.
# Legacy (CCK/OFDM rates):
#    beacon_rate=<legacy rate in 100 kbps>
# HT:
#    beacon_rate=ht:<HT MCS>
# VHT:
#    beacon_rate=vht:<VHT MCS>
# HE:
#    beacon_rate=he:<HE MCS>
#
# For example, beacon_rate=10 for 1 Mbps or beacon_rate=60 for 6 Mbps (OFDM).
#beacon_rate=10

# Short Preamble
# This parameter can be used to enable optional use of short preamble for
# frames sent at 2 Mbps, 5.5 Mbps, and 11 Mbps to improve network performance.
# This applies only to IEEE 802.11b-compatible networks and this should only be
# enabled if the local hardware supports use of short preamble. If any of the
# associated STAs do not support short preamble, use of short preamble will be
# disabled (and enabled when such STAs disassociate) dynamically.
# 0 = do not allow use of short preamble (default)
# 1 = allow use of short preamble
#preamble=1

# Station MAC address -based authentication
# Please note that this kind of access control requires a driver that uses
# hostapd to take care of management frame processing and as such, this can be
# used with driver=hostap or driver=nl80211, but not with driver=atheros.
# 0 = accept unless in deny list
# 1 = deny unless in accept list
# 2 = use external RADIUS server (accept/deny lists are searched first)
macaddr_acl=0

# Accept/deny lists are read from separate files (containing list of
# MAC addresses, one per line). Use absolute path name to make sure that the
# files can be read on SIGHUP configuration reloads.
#accept_mac_file=/etc/hostapd.accept
#deny_mac_file=/etc/hostapd.deny

# IEEE 802.11 specifies two authentication algorithms. hostapd can be
# configured to allow both of these or only one. Open system authentication
# should be used with IEEE 802.1X.
# Bit fields of allowed authentication algorithms:
# bit 0 = Open System Authentication
# bit 1 = Shared Key Authentication (requires WEP)
auth_algs=3

# Send empty SSID in beacons and ignore probe request frames that do not
# specify full SSID, i.e., require stations to know SSID.
# default: disabled (0)
# 1 = send empty (length=0) SSID in beacon and ignore probe request for
#     broadcast SSID
# 2 = clear SSID (ASCII 0), but keep the original length (this may be required
#     with some clients that do not support empty SSID) and ignore probe
#     requests for broadcast SSID
ignore_broadcast_ssid=0

# Do not reply to broadcast Probe Request frames from unassociated STA if there
# is no room for additional stations (max_num_sta). This can be used to
# discourage a STA from trying to associate with this AP if the association
# would be rejected due to maximum STA limit.
# Default: 0 (disabled)
#no_probe_resp_if_max_sta=0

# Additional vendor specific elements for Beacon and Probe Response frames
# This parameter can be used to add additional vendor specific element(s) into
# the end of the Beacon and Probe Response frames. The format for these
# element(s) is a hexdump of the raw information elements (id+len+payload for
# one or more elements)
#vendor_elements=dd0411223301

# Additional vendor specific elements for (Re)Association Response frames
# This parameter can be used to add additional vendor specific element(s) into
# the end of the (Re)Association Response frames. The format for these
# element(s) is a hexdump of the raw information elements (id+len+payload for
# one or more elements)
#assocresp_elements=dd0411223301

# TX queue parameters (EDCF / bursting)
# tx_queue_<queue name>_<param>
# queues: data0, data1, data2, data3
#		(data0 is the highest priority queue)
# parameters:
#   aifs: AIFS (default 2)
#   cwmin: cwMin (1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191,
#	   16383, 32767)
#   cwmax: cwMax (same values as cwMin, cwMax >= cwMin)
#   burst: maximum length (in milliseconds with precision of up to 0.1 ms) for
#          bursting
#
# Default WMM parameters (IEEE 802.11 draft; 11-03-0504-03-000e):
# These parameters are used by the access point when transmitting frames
# to the clients.
#
# Low priority / AC_BK = background
#tx_queue_data3_aifs=7
#tx_queue_data3_cwmin=15
#tx_queue_data3_cwmax=1023
#tx_queue_data3_burst=0
# Note: for IEEE 802.11b mode: cWmin=31 cWmax=1023 burst=0
#
# Normal priority / AC_BE = best effort
#tx_queue_data2_aifs=3
#tx_queue_data2_cwmin=15
#tx_queue_data2_cwmax=63
#tx_queue_data2_burst=0
# Note: for IEEE 802.11b mode: cWmin=31 cWmax=127 burst=0
#
# High priority / AC_VI = video
#tx_queue_data1_aifs=1
#tx_queue_data1_cwmin=7
#tx_queue_data1_cwmax=15
#tx_queue_data1_burst=3.0
# Note: for IEEE 802.11b mode: cWmin=15 cWmax=31 burst=6.0
#
# Highest priority / AC_VO = voice
#tx_queue_data0_aifs=1
#tx_queue_data0_cwmin=3
#tx_queue_data0_cwmax=7
#tx_queue_data0_burst=1.5
# Note: for IEEE 802.11b mode: cWmin=7 cWmax=15 burst=3.3

# 802.1D Tag (= UP) to AC mappings
# WMM specifies following mapping of data frames to different ACs. This mapping
# can be configured using Linux QoS/tc and sch_pktpri.o module.
# 802.1D Tag	802.1D Designation	Access Category	WMM Designation
# 1		BK			AC_BK		Background
# 2		-			AC_BK		Background
# 0		BE			AC_BE		Best Effort
# 3		EE			AC_BE		Best Effort
# 4		CL			AC_VI		Video
# 5		VI			AC_VI		Video
# 6		VO			AC_VO		Voice
# 7		NC			AC_VO		Voice
# Data frames with no priority information: AC_BE
# Management frames: AC_VO
# PS-Poll frames: AC_BE

# Default WMM parameters (IEEE 802.11 draft; 11-03-0504-03-000e):
# for 802.11a or 802.11g networks
# These parameters are sent to WMM clients when they associate.
# The parameters will be used by WMM clients for frames transmitted to the
# access point.
#
# note - txop_limit is in units of 32microseconds
# note - acm is admission control mandatory flag. 0 = admission control not
# required, 1 = mandatory
# note - Here cwMin and cmMax are in exponent form. The actual cw value used
# will be (2^n)-1 where n is the value given here. The allowed range for these
# wmm_ac_??_cwmin,cwmax is 0..15 with cwmax >= cwmin.
#
wmm_enabled=1
#
# WMM-PS Unscheduled Automatic Power Save Delivery [U-APSD]
# Enable this flag if U-APSD supported outside hostapd (eg., Firmware/driver)
#uapsd_advertisement_enabled=1
#
# Low priority / AC_BK = background
wmm_ac_bk_cwmin=4
wmm_ac_bk_cwmax=10
wmm_ac_bk_aifs=7
wmm_ac_bk_txop_limit=0
wmm_ac_bk_acm=0
# Note: for IEEE 802.11b mode: cWmin=5 cWmax=10
#
# Normal priority / AC_BE = best effort
wmm_ac_be_aifs=3
wmm_ac_be_cwmin=4
wmm_ac_be_cwmax=10
wmm_ac_be_txop_limit=0
wmm_ac_be_acm=0
# Note: for IEEE 802.11b mode: cWmin=5 cWmax=7
#
# High priority / AC_VI = video
wmm_ac_vi_aifs=2
wmm_ac_vi_cwmin=3
wmm_ac_vi_cwmax=4
wmm_ac_vi_txop_limit=94
wmm_ac_vi_acm=0
# Note: for IEEE 802.11b mode: cWmin=4 cWmax=5 txop_limit=188
#
# Highest priority / AC_VO = voice
wmm_ac_vo_aifs=2
wmm_ac_vo_cwmin=2
wmm_ac_vo_cwmax=3
wmm_ac_vo_txop_limit=47
wmm_ac_vo_acm=0
# Note: for IEEE 802.11b mode: cWmin=3 cWmax=4 burst=102

# Enable Multi-AP functionality
# 0 = disabled (default)
# 1 = AP support backhaul BSS
# 2 = AP support fronthaul BSS
# 3 = AP supports both backhaul BSS and fronthaul BSS
#multi_ap=0

# Static WEP key configuration
#
# The key number to use when transmitting.
# It must be between 0 and 3, and the corresponding key must be set.
# default: not set
#wep_default_key=0
# The WEP keys to use.
# A key may be a quoted string or unquoted hexadecimal digits.
# The key length should be 5, 13, or 16 characters, or 10, 26, or 32
# digits, depending on whether 40-bit (64-bit), 104-bit (128-bit), or
# 128-bit (152-bit) WEP is used.
# Only the default key must be supplied; the others are optional.
# default: not set
#wep_key0=123456789a
#wep_key1="vwxyz"
#wep_key2=0102030405060708090a0b0c0d
#wep_key3=".2.4.6.8.0.23"



'''
