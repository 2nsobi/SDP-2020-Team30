import subprocess
import re


class WindowsSoftAP:
    def __init__(self, ssid="test_ap_1", key="12345678"):
        self.ssid = ssid
        self.key = key
        self.ipv4 = None

    def start_ap(self):
        print("\n*** CONFIGURING WINDOWS HOSTEDNETWORK ***\n")
        run_cmd(f"netsh wlan set hostednetwork mode=allow ssid={self.ssid} key={self.key}")

        print("\n*** STARTING WINDOWS HOSTEDNETWORK ***\n")
        run_cmd("netsh wlan start hostednetwork")

        print("\n*** RETRIEVING IP OF WINDOWS HOSTEDNETWORK ***\n")
        self.ipv4 = self._get_ipv4()
        print(f"*** WINDOWS HOSTEDNETWORK IP: {self.ipv4} ***")

    def get_ipv4(self):
        if not self.ipv4:
            self.ipv4 = self._get_ipv4()
            if not self.ipv4:
                print('this WindowsSoftAP instance has no associated IP, must run WindowsSoftAP.start_ap() first')
            else:
                return self.ipv4
        else:
            return self.ipv4

    def stop_ap(self):
        print("\n*** STOPPING WINDOWS HOSTEDNETWORK ***\n")
        run_cmd("netsh wlan stop hostednetwork")

    def _get_ipv4(self):
        stdout = run_cmd("ipconfig /all", show=False)
        record_next_ip = False
        ip = None
        for line in stdout.lower().split(sep='\n'):
            if not record_next_ip:
                if "microsoft hosted network virtual adapter" in line:
                    record_next_ip = True
            else:
                if "ipv4 address" in line:
                    match = re.search(r'(\d+)\.(\d+)\.(\d+)\.(\d+)', line)
                    ip = match.group(0)
                    break
        return ip


def run_cmd(cmd_str, show=True):
    proc = subprocess.run(cmd_str.split(), capture_output=True, encoding="utf-8")
    if proc.returncode != 0:
        print(f'*** ERROR occurred during subprocess.run call, command: ***\n{" ".join(proc.args)}'
              f'\n*** stderr: ***\n{proc.stdout.strip()}')
        raise RuntimeError
    else:
        if show:
            print(proc.stdout.strip())
    return proc.stdout.strip()


if __name__ == "__main__":
    ap = WindowsSoftAP()
    ap.start_ap()
    ap.stop_ap()
