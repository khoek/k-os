import sys

decl_template = "DECLARE_SYSCALL({});"
decl_template2 = "DECLARE_SYSCALL({}, {});"

outfile = open(sys.argv[2], 'w')
for line in open(sys.argv[1], 'r'):
  line = line.strip()
  if len(line) > 0:
    id, name, args = line.strip().split(":")
    if len(args) == 0:
      outfile.write(decl_template.format(name))
    else:
      outfile.write(decl_template2.format(name, args))
  outfile.write("\n")
