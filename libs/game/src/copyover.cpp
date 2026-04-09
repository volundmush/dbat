


void game_info(const char *format, ...) 
{ 
  struct descriptor_data *i; 
  va_list args; 
  char messg[MAX_STRING_LENGTH]; 

  if (format == NULL) 
    return; 

  sprintf(messg, "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W"); 

  for (i = descriptor_list; i; i = i->next) { 
    if (STATE(i) != CON_PLAYING && (STATE(i) != CON_REDIT && STATE(i) != CON_OEDIT && STATE(i) != CON_MEDIT)) 
      continue; 
    if (!(i->character)) 
      continue; 

    write_to_output(i, messg); 
    va_start(args, format); 
    vwrite_to_output(i, format, args); 
    va_end(args); 
    write_to_output(i, "@n\r\n@R>>>@GMake sure to pick up your bed items and save.@n\r\n"); 
  } 
}
