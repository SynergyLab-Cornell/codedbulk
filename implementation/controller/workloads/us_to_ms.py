import argparse
from ctypes import c_longlong as ll

parser = argparse.ArgumentParser()
parser.add_argument('f', metavar='Filename', type=str, nargs='+')
args = parser.parse_args()

print args

for name in args.f:
   file = open(name,"r")
   file_out = open(name+"_out","w")
   file_out.write(file.readline())
   for line in file:
      int_flow_id, int_time_us, int_src_id, int_dst_id, int_flow_size = line.split(" ")
      int_time_ms = int(int(int_time_us)/1000)
      file_out.write('{} {} {} {} {}'.format(int_flow_id, int_time_ms, int_src_id, int_dst_id, int_flow_size))
   