def bytes_ip_to_ip_str(btyes_str, byte_order='little', signed=False, ipv4=True):
    ip_integer = int.from_bytes(btyes_str, byteorder=byte_order, signed=signed)
    ip_str = int_ip_to_ip_str(ip_integer, ipv4=ipv4)
    return ip_str


def int_ip_to_ip_str(ip_integer, ipv4=True):
    ip_byte_strings = []
    if ipv4:
        for i in range(3, -1, -1):
            byte = (ip_integer >> (i * 8)) & 0xFF
            ip_byte_strings.append(str(byte))
        ip_str = '.'.join(ip_byte_strings)
        return ip_str
    else:
        return None


def hex_ip_to_ip_str(hex_str, ipv4=True):
    if ipv4:
        if len(hex_str) != 8:
            print(f'hex string has a length greater than 8: len={len(hex_str)}')
            return None

        ip_byte_strings = []
        for i in range(0, len(hex_str), 2):
            byte = str(int(hex_str[i:i + 2], base=16))
            ip_byte_strings.append(byte)
        ip_str = '.'.join(ip_byte_strings)
        return ip_str
    else:
        return None


if __name__ == "__main__":
    print(bytes_ip_to_ip_str(b'\xe8\x89\xa8\xc0'))
    print(hex_ip_to_ip_str('c0a889df'))
    print(int_ip_to_ip_str(3232270634))
