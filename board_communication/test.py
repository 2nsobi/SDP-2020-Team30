import datetime
import os

a = "b'asfdasdfsad'"

b = a.find("b'")
print(b)
c = a[b+2:]
print(c)
d = a.replace("b'", "")
print("d: {}".format(d))