conf_file = '''

#####Sets WPA and WPA2 authentication#####
#wpa option sets which wpa implementation to use
#1 - wpa only
#2 - wpa2 only
#3 - both
wpa={}
#sets wpa passphrase required by the clients to authenticate themselves on the network
{}wpa_passphrase={}
#sets wpa key management
{}wpa_key_mgmt=WPA-PSK
#sets encryption used by WPA
{}wpa_pairwise=TKIP
#sets encryption used by WPA2
{}rsn_pairwise=CCMP

own_ip_addr={}

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

driver={}


# Interface for separate control program. If this is specified, hostapd
# will create this directory and a UNIX domain socket for listening to requests
# from external programs (CLI/GUI, etc.) for status information and
# configuration. The socket file will be named based on the interface name, so
# multiple hostapd processes/interfaces can be run at the same time if more
# than one interface is used.
# /var/run/hostapd is the recommended directory for sockets and by default,
# hostapd_cli will use it when trying to connect with hostapd.
#ctrl_interface=/var/run/hostapd

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

hw_mode=g

channel={}

# Beacon interval in kus (1.024 ms) (default: 100; range 15..65535)
beacon_int={}

# Station MAC address -based authentication
# Please note that this kind of access control requires a driver that uses
# hostapd to take care of management frame processing and as such, this can be
# used with driver=hostap or driver=nl80211, but not with driver=atheros.
# 0 = accept unless in deny list
# 1 = deny unless in accept list
# 2 = use external RADIUS server (accept/deny lists are searched first)
macaddr_acl=0

# IEEE 802.11 specifies two authentication algorithms. hostapd can be
# configured to allow both of these or only one. Open system authentication
# should be used with IEEE 802.1X.
# Bit fields of allowed authentication algorithms:
# bit 0 = Open System Authentication
# bit 1 = Shared Key Authentication (requires WEP)
{}auth_algs={}

'''
