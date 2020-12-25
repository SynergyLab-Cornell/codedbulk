import argparse

parser = argparse.ArgumentParser()
parser.add_argument('f', metavar='Filename', type=str, nargs='+')
args = parser.parse_args()

print args

for name in args.f:
   file = open(name,"r")
   file_out = open(name+"_out","w")
   file_out.write(file.readline())
   for line in file:
      int_flow_id, int_time_ms, tmp1, tmp2, tmp3, int_src_id, int_dst_id, int_flow_size = line.split(" ")
      file_out.write('{} {} {} {} {}'.format(int_flow_id, int_time_ms, int_src_id, int_dst_id, int_flow_size))
   