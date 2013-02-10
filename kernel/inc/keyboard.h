void keyboard_init();
void keyboard_register_key_up(void (*handler)(char));
void keyboard_register_key_down(void (*handler)(char));
