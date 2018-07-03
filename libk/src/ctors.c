typedef void (*ctp)();
void
__do_global_ctors ()
{
  extern int __CTOR_LIST__;
  int *c = &__CTOR_LIST__;
  c++;
  while (*c)
    {
      ctp d = (ctp)*c;
      (d)();
      c++;
    }
}

void
__do_global_dtors ()
{
  extern int __DTOR_LIST__;
  int *c = &__DTOR_LIST__;
  int *cp = c;
  c++;
  while (*c)
    {
      c++;
    }
  c--;
  while (c > cp)
    {
      ctp d = (ctp)*c;
      (*d)();
      c--;
    }
}
