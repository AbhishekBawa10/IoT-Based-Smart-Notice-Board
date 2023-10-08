#include <ESP8266WiFi.h> // MD_MAX72XX library implements the hardware-specific functions of the LED matrix.
#include <MD_Parola.h> // MD_Parola library implements the text effect.
#include <MD_MAX72xx.h> // The library implements functions that allow the MAX72xx to be used for LED matrices.
#include <SPI.h> //SPI library which is used to communicate with the display via SPI.
//#include <ESP8266mDNS.h>

// Turn on debug statements to the serial output
#define  DEBUG  0
#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif
 // specify which hardware is being used
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // Set the HARDWARE_TYPE to FC16_HW, if you’re using a module with a blue PCB
#define MAX_DEVICES 8  // MAX_DEVICES, specifies the number of MAX7219 ICs being used.
#define CS_PIN    15 // or SS
 
// HARDWARE SPI
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES); // function MD_Parola() is then used to create a new instance of the MD_Parola class.
 
// WiFi login parameters - network name and password
const char* ssid = "POCOWiFi";
const char* password = "ab210423";
 //MDNSResponder mdns;
// WiFi Server object and parameters
WiFiServer server(80);

// Scrolling parameters
uint8_t frameDelay = 25;  // default frame delay value
textEffect_t  scrollEffect = PA_SCROLL_LEFT;
 
// Global message buffers shared by Wifi and Scrolling functions
#define BUF_SIZE  512
char curMessage[BUF_SIZE];
char newMessage[BUF_SIZE];
bool newMessageAvailable = false;
 
const char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
 
const char WebPage[] =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<center>\n"
"<style>\n"
"body {\n"
"background: linear-gradient(to right, #16bffd, #cb3066);\n"
"width: 90%;\n"
"height: 100vh;\n"
"background-position: center;\n"
"background-size: cover;\n"
"display: flex;\n"
"align-items: center;\n"
"justify-content: center;\n"
"}\n"
" .card{\n"
"width: 100%;\n"
"max-width: 800px;\n"
"color: #fff;\n"
"text-align: center;\n"
"padding: 50px 35px;\n"
"border: 1px solid rgba(255,255,255,0.3);\n"
"background:rgba(255,255,255,0.2);\n"
"border-radius: 25px;\n"
"border-shadow:0 4px 30px rgba(0,0,0,0.1);\n"
"backdrop-filter:blur(5px);\n"
"}\n"
".card h2{\n"
"font-size: 40px;\n"
"font-weight: 600;\n"
"margin-top: 20px;\n"
"}\n"
".card h1{\n"
"font-size: 22px;\n"
"margin: 10px auto;\n"
"max-width: 500px;\n"
"}\n"
".card h{\n"
"font-size: 18px;\n"
"margin: 10px auto;\n"
"max-width: 500px;\n"
"font-weight: 500;\n"
"}\n"
".card input[type=submit]{\n"
"text-decoration: none;\n"
"display: inline-block;\n"
"font-size: 18px;\n"
"font-weight: 500;\n"
"background: linear-gradient(to right, #00c6ff, #0072ff);\n"
"color:#FFFF;\n"
"padding: 10px 30px;\n"
"border-radius: 30px;\n"
"margin: 30px 0 10px;\n"
"border: none;\n"
"\n"
"}\n"
".card input[type=text], select {\n"
"  width: 70%;\n"
"  padding: 12px 20px;\n"
"  margin: 8px 0;\n"
"  display: inline-block;\n"
"  border: 1px solid #ccc;\n"
"  border-radius: 10px;\n"
"  box-sizing: border-box;\n"
"}\n"
"</style>\n"
"<center>\n"
"<title>Smart Notice Board</title>\n"
" \n"
"<script>" \
"strLine = \"\";" \
 
"function SendData()" \
"{" \
"  nocache = \"/&nocache=\" + Math.random() * 1000000;" \
"  var request = new XMLHttpRequest();" \
"  strLine = \"&MSG=\" + document.getElementById(\"data_form\").Message.value;" \
"  strLine = strLine + \"/&SD=\" + document.getElementById(\"data_form\").ScrollType.value;" \
"  strLine = strLine + \"/&I=\" + document.getElementById(\"data_form\").Invert.value;" \
"  strLine = strLine + \"/&SP=\" + document.getElementById(\"data_form\").Speed.value;" \
"  request.open(\"GET\", strLine + nocache, false);" \
"  request.send(null);" \
"}" \
"</script>" \
"</head>\n"
" \n"
"<body>\n"
"\n"
"<div class=\"card\">\n"
"<h>SNJB's K.B.Jain College of Engineering</h>\n"
"<h1><b>Electronics And Telecommunication Department</b></h1>\n"
"<h2><b>Smart Notice Board</b></h2>\n"
" \n"
"<form id=\"data_form\" name=\"frmText\">" \
"<label><h>Enter Notice:</h><br><input type=\"text\" name=\"Message\" maxlength=\"255\"></label>\n"
"<br><br>\n"
"<input type = \"radio\" name = \"Invert\" value = \"0\" checked> <h>Normal</h>\n"
"<input type = \"radio\" name = \"Invert\" value = \"1\"> <h>Inverse</h>\n"
"<br>\n"
"<input type = \"radio\" name = \"ScrollType\" value = \"L\" checked> <h>Left Scroll</h>\n"
"<input type = \"radio\" name = \"ScrollType\" value = \"R\" > <h>Right Scroll</h>\n"
"<br><br>\n"
"<label><h>Speed:</h><br><h>Fast</h><input type=\"range\" name=\"Speed\"min=\"10\" max=\"200\"><h>Slow</h>\n"
"<br>\n"
"</form>\n"
"<br>\n"
"<input type=\"submit\" value=\"Send Notice\" onclick=\"SendData()\">" \
"</div>\n"
"\n"
"</body>\n"
"</center>\n"
"</html>";
 
const char *err2Str(wl_status_t code)
{
  switch (code)
  {
  case WL_IDLE_STATUS:    return("IDLE");           break; // WiFi is in process of changing between statuses
  case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; // case configured SSID cannot be reached
  case WL_CONNECTED:      return("CONNECTED");      break; // successful connection is established
  case WL_CONNECT_FAILED: return("CONNECT_FAILED"); break; // password is incorrect
  case WL_DISCONNECTED:   return("CONNECT_FAILED"); break; // module is not configured in station mode
  default: return("??");
  }
}
 
uint8_t htoi(char c)
{
  c = toupper(c);
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A' + 0xa);
  return(0);
}
 
void getData(char *szMesg, uint16_t len)
// Message may contain data for:
// New text (/&MSG=)
// Scroll direction (/&SD=)
// Invert (/&I=)
// Speed (/&SP=)
{
  char *pStart, *pEnd;      // pointer to start and end of text
 
  // check text message
  pStart = strstr(szMesg, "/&MSG=");
  if (pStart != NULL)
  {
    char *psz = newMessage;
 
    pStart += 6;  // skip to start of data
    pEnd = strstr(pStart, "/&");
 
    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        if ((*pStart == '%') && isxdigit(*(pStart + 1)))
        {
          // replace %xx hex code with the ASCII character
          char c = 0;
          pStart++;
          c += (htoi(*pStart++) << 4);
          c += htoi(*pStart++);
          *psz++ = c;
        }
        else
          *psz++ = *pStart++;
      }
 
      *psz = '\0'; // terminate the string
      newMessageAvailable = (strlen(newMessage) != 0);
      PRINT("\nNew Msg: ", newMessage);
    }
  }
 
  // check scroll direction
  pStart = strstr(szMesg, "/&SD=");
  if (pStart != NULL)
  {
    pStart += 5;  // skip to start of data
 
    PRINT("\nScroll direction: ", *pStart);
    scrollEffect = (*pStart == 'R' ? PA_SCROLL_RIGHT : PA_SCROLL_LEFT);
    P.setTextEffect(scrollEffect, scrollEffect);
    P.displayReset();
  }
 
  // check invert
  pStart = strstr(szMesg, "/&I=");
  if (pStart != NULL)
  {
    pStart += 4;  // skip to start of data
 
    PRINT("\nInvert mode: ", *pStart);
    P.setInvert(*pStart == '1');
  }
 
  // check speed
  pStart = strstr(szMesg, "/&SP=");
  if (pStart != NULL)
  {
    pStart += 5;  // skip to start of data
 
    int16_t speed = atoi(pStart);
    PRINT("\nSpeed: ", P.getSpeed());
    P.setSpeed(speed);
    frameDelay = speed;
  }
}
 
void handleWiFi(void)
{
  static enum { S_IDLE, S_WAIT_CONN, S_READ, S_EXTRACT, S_RESPONSE, S_DISCONN } state = S_IDLE;
  static char szBuf[1024];
  static uint16_t idxBuf = 0;
  static WiFiClient client;
  static uint32_t timeStart;
 
  switch (state)
  {
  case S_IDLE:   // initialise
    PRINTS("\nS_IDLE");
    idxBuf = 0;
    state = S_WAIT_CONN;
    break;
 
  case S_WAIT_CONN:   // waiting for connection
  {
    client = server.available();
    if (!client) break;
    if (!client.connected()) break;
 
#if DEBUG
    char szTxt[20];
    sprintf(szTxt, "%03d:%03d:%03d:%03d", client.remoteIP()[0], client.remoteIP()[1], client.remoteIP()[2], client.remoteIP()[3]);
    PRINT("\nNew client @ ", szTxt);
#endif
 
    timeStart = millis();
    state = S_READ;
  }
  break;
 
  case S_READ: // get the first line of data
    PRINTS("\nS_READ ");
 
    while (client.available())
    {
      char c = client.read();
 
      if ((c == '\r') || (c == '\n'))
      {
        szBuf[idxBuf] = '\0';
        client.flush();
        PRINT("\nRecv: ", szBuf);
        state = S_EXTRACT;
      }
      else
        szBuf[idxBuf++] = (char)c;
    }
    if (millis() - timeStart > 1000)
    {
      PRINTS("\nWait timeout");
      state = S_DISCONN;
    }
    break;
 
  case S_EXTRACT: // extract data
    PRINTS("\nS_EXTRACT");
    // Extract the string from the message if there is one
    getData(szBuf, BUF_SIZE);
    state = S_RESPONSE;
    break;
 
  case S_RESPONSE: // send the response to the client
    PRINTS("\nS_RESPONSE");
    // Return the response to the client (web page)
    client.print(WebResponse);
    client.print(WebPage);
    state = S_DISCONN;
    break;
 
  case S_DISCONN: // disconnect client
    PRINTS("\nS_DISCONN");
    client.flush();
    client.stop();
    state = S_IDLE;
    break;
 
  default:  state = S_IDLE;
  }
}
 
void setup()
{
  Serial.begin(57600);
 
 
  P.begin(); //  function begin() to initialize the object
  P.setIntensity(0); // The brightness of the display can be adjusted using the function setIntensity() ranging from 0 to 15.
  P.displayClear(); // display is cleared using the displayClear() function.
  P.displaySuspend(false);
 // this function takes four arguments displayScroll(pText, align, textEffect, speed)
  P.displayScroll(curMessage, PA_LEFT, scrollEffect, frameDelay);
 
  curMessage[0] = newMessage[0] = '\0';
 
  // Connect to and initialise WiFi network
  PRINT("\nConnecting to ", ssid);
 
  WiFi.begin(ssid, password);
  /*
  if (mdns.begin("esp", WiFi.localIP())){
    Serial.println("MDNS responder started");
  }
   //MDNS.addService("http", "tcp", 80);
   */
  while (WiFi.status() != WL_CONNECTED)
  {
    PRINT("\n", err2Str(WiFi.status()));
    delay(500);
  }
  PRINTS("\nWiFi connected");
 
  // Start the server
  server.begin();
  PRINTS("\nServer started");
 
  // Set up first message as the IP address
  sprintf(curMessage, "%03d:%03d:%03d:%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  PRINT("\nAssigned IP ", curMessage);
}
 
void loop()
{
  handleWiFi();
 
  if (P.displayAnimate()) // This function scrolls the text and returns true when it is finished.
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset(); // When the scrolling is finished,we use the function displayReset() to reset the display,resulting in continuous scrolling.
  }
}
