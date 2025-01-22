
void data_file_init(void) {
  Serial.println("Finding next file...");
  //todo : number for file
  logFileName = "/COMETS01.txt";
  // Create the new file
  logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    logFile.println("COMETS_DATABASE_FILE_FORMAT");  // Add header row if needed
    logFile.print("version: ");
    logFile.println(COMETS_FILE_VERSION, DEC);
    logFile.println("#sensor_tab");

    //TODO : trouver un endroit ou charger c'est infos
    char* sensor_list[] = { "Motor",
                            "Cable 1",
                            "Cable 2",
                            "Cable 3" };
    int nbr_sensor = 4;
    int is_activated = 1;
    for (int i = 0; i < nbr_sensor; i++) {
      logFile.print("- ");
      logFile.print(i, DEC);
      logFile.print(", ");
      logFile.print(sensor_list[i]);
      logFile.println(is_activated, DEC);
    }
    logFile.println("#END_OF_TABLE");
    logFile.println("log_data_table");

    //TODO : trouver un endroit ou charger c'est infos
    char* table_list[] = { "#Temparature ",
                           "#Volatge ",
                           "#RPM ",
                           "#AMPS ",
                           "#Error " };
    char table_argument[] = { 't', 'v', 'r', 'a', 'e' };
    int nbr_table = 5;
    for (int i = 0; i < nbr_table; i++) {
      logFile.print(table_list[i]);
      logFile.print("-");
      logFile.println(table_argument);
    }
    logFile.println("----Starting_log_now----");
  } else {
    Serial.println("Failed to create log file");
  }
}

void save_data(char type_data, uint8_t id_sensor, uint32_t data, uint32_t time_nbr) {
  logFile.print("-");
  logFile.print(type_data);
  logFile.print(", ");
  logFile.print(id_sensor, HEX);
  logFile.print(", ");
  logFile.print(data, HEX);
  logFile.print(", ");
  logFile.println(time_nbr, HEX);
  Serial.print("-");
  Serial.print(type_data);
  Serial.print(", ");
  Serial.print(id_sensor, HEX);
  Serial.print(", ");
  Serial.print(data, HEX);
  Serial.print(", ");
  Serial.println(time_nbr, HEX);
#ifdef TELNET_USE
  if (is_telnet_user_connected) {
    telnet.print("-");
    telnet.print(type_data);
    telnet.print(", ");
    telnet.print(id_sensor, HEX);
    telnet.print(", ");
    telnet.print(data, HEX);
    telnet.print(", ");
    telnet.println(time_nbr, HEX);
  }
#endif
}