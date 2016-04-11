#include "ntp.h"

// make this a class
// NOTE: call now() in loop to do syncing

// https://github.com/PaulStoffregen/Time/blob/master/examples/TimeNTP/TimeNTP.ino


const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing
                                    // packets
WiFiUDP Udp;
unsigned int localPort = 8888;          // local port to listen for UDP packets
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov

int timezone = 0; // GMT/UTC

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime() {
  while (Udp.parsePacket() > 0)
    ; // discard any previously received packets
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timezone * SECS_PER_HOUR;
    }
  }
  return 0; // return 0 if unable to get the time
}

// 2014-12-31 23:59:59 UTC
String ntpTimestamp() {
  String tz = timezone == 0 ? " UTC" : (" TZ" + ((timezone > 0 ? "+" : "") + String(timezone)));
  return String(year()) + "-" + zeroPad(month()) + "-" + zeroPad(day()) + " " +
         zeroPad(hour()) + ":" + zeroPad(minute()) + ":" + zeroPad(second()) +
         tz;
}

String ntpTimestamp(unsigned long t) {
  String tz = timezone == 0 ? " UTC" : (" TZ" + ((timezone > 0 ? "+" : "") + String(timezone)));
  return String(year(t)) + "-" + zeroPad(month(t)) + "-" + zeroPad(day(t)) +
         " " + zeroPad(hour(t)) + ":" + zeroPad(minute(t)) + ":" +
         zeroPad(second(t)) + tz;
}

void ntpBegin() { // default interval is 5 mins
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
}

void ntpBegin(time_t interval) {
  Udp.begin(localPort);
  setSyncInterval(interval);
  setSyncProvider(getNtpTime);
}
