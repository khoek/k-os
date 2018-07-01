import sys

head = "#include <k/syscall.h>\n\n"
hook_template = """.global SYSCALL({})
SYSCALL({}):
mov ${}, %eax
jmp perform_syscall_{}
"""

outfile = open(sys.argv[2], 'w')
outfile.write(head)

for line in open(sys.argv[1], 'r'):
  line = line.strip()
  if len(line) > 0:
    id, name, args = line.strip().split(":")
    arg_num = args.count(",") + 1 if len(args.strip()) != 0 else 0
    outfile.write(hook_template.format(name, name, id, arg_num))
  outfile.write("\n")
