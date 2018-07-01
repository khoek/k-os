import sys

ent_template = "[{}] = (syscall_t) (void *) SYSCALL({}),"

outfile = open(sys.argv[2], 'w')
for line in open(sys.argv[1], 'r'):
  line = line.strip()
  if len(line) > 0:
    id, name, args = line.strip().split(":")
    outfile.write(ent_template.format(id, name))
  outfile.write("\n")
