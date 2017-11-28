/*
*
* A Java server made using the Java UDP Client as a base.
* Group 13: Wulfy Boothe, Lane Little, David Harris
*/

import java.net.*; // for DatagramSocket, DatagramPacket, and InetAddress
import java.io.*; // for IOException
import java.text.DecimalFormat; //for Formating
import java.nio.ByteBuffer;


public class ServerUDP { 

   /*
   Does note appear useful in this lab.
   private static final int TIMEOUT = 3000; // Resend timeout (milliseconds) 
   private static final int MAXTRIES = 7; // Maximum retransmissions 
   */
   private static final int MAX_MESSAGE_LENGTH = 7;
   private static final int MAGICNUM = 0x4A6F7921;
   public static void main(String[] args) throws IOException {
   
      if (args.length != 1) 
      {
      // Test for correct # of args     
         System.err.println("Parameter(s): <Servername> <Port#>");
         return; 
      }
      boolean waitingClient = false;
      int servPort = Integer.parseInt(args[0]);
      short recPort = 0;
      byte clientGID = -1;
      long clientIP = 0;
      
      try{ //try block for attempting to send and recieve the message
         DatagramSocket socket = new DatagramSocket(servPort);
         DatagramPacket inPacket = 
            new DatagramPacket(new byte[MAX_MESSAGE_LENGTH], MAX_MESSAGE_LENGTH);
      
         while(true)
         {
            boolean genErr = false;
            boolean mnErr = false;
            boolean portErr = false;
            boolean lenErr = false;
            socket.receive(inPacket);
            //inPacket format: 
            //4 bytes magicnum 0-3
            //ph (MSByte of port num)4
            //pl (LSByte of porn num)5
            //GID (byte)6
            if (inPacket.getLength() != 7)
            {
               genErr = true;
               lenErr = true;
            }
            int recMagicNum = inPacket.getData()[0] << 24 
               | inPacket.getData()[1] << 16 | inPacket.getData()[2] << 8 
               | inPacket.getData()[3];
            if (recMagicNum != MAGICNUM)
            {
               genErr = true;
               mnErr = true;
               
               
            }
            short portCheck = (short) ((inPacket.getData()[4] << 8) | inPacket.getData()[5]);
            clientGID = inPacket.getData()[6];
            boolean tooBig = portCheck > (10010 + (5*clientGID + 4));
            boolean tooSmall = portCheck < (10010 + (5*clientGID));
            
            byte[] response;
            int TML = 0;
            if(tooSmall || tooBig)
            {
               genErr = true;
               portErr = true;
            }
            if (genErr)
            {
               TML = 7;
               byte XY = 0;
               if (mnErr)
               {
                  XY |= 0x1;
               }
               if (lenErr)
               {
                  XY |= 0x2;
               }
               if (portErr)
               {
                  XY |= 0x4;
               }
               response = new byte[TML];
               response[0] = (byte)(MAGICNUM >> 24);
               response[1] = (byte)(MAGICNUM >> 16);
               response[2] = (byte)(MAGICNUM >> 8);
               response[3] = (byte) MAGICNUM;
               response[4] = 13;
               response[5] = 00;
               response[6] = XY;
               DatagramPacket outPacket = new DatagramPacket(response, TML);
               socket.send(outPacket);
               inPacket.setLength(MAX_MESSAGE_LENGTH); 
               continue;
            }
         
            
            if (waitingClient)
            {  
               TML = 11;
               response = new byte[TML];
               response[0] = (byte)(MAGICNUM >> 24);
               response[1] = (byte)(MAGICNUM >> 16);
               response[2] = (byte)(MAGICNUM >> 8);
               response[3] = (byte) MAGICNUM;
               response[4] = (byte) (clientIP >> 24);
               response[5] = (byte) (clientIP >> 16);
               response[6] = (byte) (clientIP >> 8);
               response[7] = (byte) clientIP;
               response[8] = (byte) (recPort >> 8);
               response[9] = (byte) recPort;
               response[10] = 13;
               DatagramPacket outPacket = new DatagramPacket(response, TML);
               socket.send(outPacket);
               //Forget old data
               recPort = 0;
               clientGID = -1;
               clientIP = 0;
               waitingClient = false;
            }
            else
            {
               InetAddress addr = socket.getInetAddress();
               clientIP = getIP32(addr.getAddress());
               TML = 7;
               recPort = (short) (inPacket.getData()[4] << 8 | inPacket.getData()[5]);
               response = new byte[TML];
               response[0] = (byte)(MAGICNUM >> 24);
               response[1] = (byte)(MAGICNUM >> 16);
               response[2] = (byte)(MAGICNUM >> 8);
               response[3] = (byte) MAGICNUM;
               response[4] = 13;
               response[5] = inPacket.getData()[4]; 
               response[6] = inPacket.getData()[5];
               DatagramPacket outPacket = new DatagramPacket(response, TML);
               socket.send(outPacket);
               waitingClient = true;
            }
            inPacket.setLength(MAX_MESSAGE_LENGTH);
         }
            
      }
      catch (UnknownHostException uhe)
      {
         System.err.println("Failed to find host.");
         return;
      }
      catch (SocketException se)
      {
         System.err.println("Failed to create socket.");
         return;
      }  
   }     
   
   static long getIP32(byte[] bytes) {
      int val = 0;
      for (int i = 0; i < bytes.length; i++) {
         val <<= 8;
         val |= bytes[i] & 0xff;
      }
      long result = ((long)val)&0xffffffff;
      return result;
   }            
}

