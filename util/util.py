import subprocess


def bytes_ip_to_ip_str(btyes_str, byte_order='little', signed=False, ipv4=True):
    """
    Converts a string of bytes, assumed to be an IP address from the , to an IP string w/ the bytes of the
    IP separated by dots (xxx.xxx.xxx.xxx). Currently only implemented for IPv4 addresses

    :param btyes_str: (str) string of bytes in the form: "b'\xe8\x89\xa8\xc0'"
    :param byte_order: (str) byte order the bytes string should be decoded from passed to python's int.from_bytes() func
    :param signed: (bool) specifies if the bytes string in signed or not and pass to int.from_bytes()
    :param ipv4: (bool) indicates if the IP should be decoded as an IPv4 or IPv6 address
    :return: (str) the IP string w/ the bytes of the IP separated by dots (xxx.xxx.xxx.xxx)
    """

    ip_integer = int.from_bytes(btyes_str, byteorder=byte_order, signed=signed)
    ip_str = int_ip_to_ip_str(ip_integer, ipv4=ipv4)
    return ip_str


def int_ip_to_ip_str(ip_integer, ipv4=True):
    """
    Converts an IP in the form of an integer, from the CC3220SF MCU, to an IP string. Currently only
    implemented for IPv4 addresses

    :param ip_integer: (int) integer represeting an IP from the
    :param ipv4: (bool) indicates if the IP should be decoded as an IPv4 or IPv6 address
    :return: (str) the IP string w/ the bytes of the IP separated by dots (xxx.xxx.xxx.xxx)
    """
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
    """
    Converts a hex string representing an IP address, from the CC3220SF MCU, to an IP string. Currently only
    implemented for IPv4 addresses

    :param hex_str: (str) hex string representing IP in the form: "c0a889df"
    :param ipv4: (bool) indicates if the IP should be decoded as an IPv4 or IPv6 address
    :return: (str) the IP string w/ the bytes of the IP separated by dots (xxx.xxx.xxx.xxx)
    """
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


def run_cmd(cmd_str, show=True):
    """
    Runs the shell command specified in cmd_str w/ subprocess.run and will raise an error and print stacktrace on error
    of the command, otherwise prints the stdout of the command

    :param cmd_str: (str) shell command
    :param show: (bool) when true prints the stdout of the shell command on success, otherwise prints nothing
    :return: (str) the stdout of the shell command on success as a unicode string
    """

    proc = subprocess.run(cmd_str.split(), capture_output=True, encoding="utf-8")
    if proc.returncode != 0:
        print(f'*** ERROR occurred during subprocess.run call, command: ***\n{" ".join(proc.args)}'
              f'\n*** stderr: ***\n{proc.stdout.strip()}')
        raise RuntimeError
    else:
        if show:
            print(proc.stdout.strip())
    return proc.stdout.strip()


def strip_end_bytes(bytes_str):
    decoded_str = str(bytes_str.split(b'\x00', 1)[0],
                      encoding='utf-8')
    return decoded_str


if __name__ == "__main__":
    print(bytes_ip_to_ip_str(b'\xe8\x89\xa8\xc0'))
    print(hex_ip_to_ip_str('c0a889df'))
    print(int_ip_to_ip_str(3232270634))
