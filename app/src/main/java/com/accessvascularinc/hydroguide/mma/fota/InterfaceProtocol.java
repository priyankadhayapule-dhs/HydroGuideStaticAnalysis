package com.accessvascularinc.hydroguide.mma.fota;

import com.accessvascularinc.hydroguide.mma.serial.Packet;
import com.accessvascularinc.hydroguide.mma.serial.SerialFrameBuilder;
import com.accessvascularinc.hydroguide.mma.serial.SerialFrameCRC;
import com.accessvascularinc.hydroguide.mma.serial.SerialFrameParser;
import com.accessvascularinc.hydroguide.mma.serial.SerialFrameStuffing;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

public class InterfaceProtocol
{
    public static final byte STX = SerialFrameStuffing.STX;
    public static final int HEADER_SIZE = 5;  // STX + ID + TS(2) + LEN
    public static final int CRC_SIZE = 2;
    public static final int OVERHEAD = HEADER_SIZE + CRC_SIZE;  // 7 bytes

    /**
     * Packet types for DFU/FOTA protocol
     */
    public static class PacketType {
        public static final int KEEP_ALIVE = 0x11;
        public static final int DFU = 0x20;
        
        public static String getName(int value) {
            switch (value) {
                case KEEP_ALIVE: return "KEEP_ALIVE";
                case DFU: return "DFU";
                default: return String.format("Unknown(0x%02X)", value);
            }
        }
    }
    
    /**
     * Packet subtypes - matches SerialFrameParser.PacketSubId
     */
    public static class PacketSubtype {
        public static final int DATA = SerialFrameParser.PacketSubId.Data.ordinal();
        public static final int COMMAND = SerialFrameParser.PacketSubId.Command.ordinal();
        
        public static String getName(int value) {
            SerialFrameParser.PacketSubId subId = null;
            for (SerialFrameParser.PacketSubId id : SerialFrameParser.PacketSubId.values()) {
                if (id.ordinal() == value) {
                    subId = id;
                    break;
                }
            }
            if (subId != null) {
                return subId.name().toUpperCase();
            }
            return String.format("Unknown(0x%02X)", value);
        }
    }
    
    /**
     * Decoded packet structure - wraps Packet from serial package with additional validation info
     */
    public static class DecodedPacket {
        public final int packetId;
        public final long timestamp;
        public final byte[] payload;
        public final boolean valid;
        public final int type;
        public final int subtype;
        public final String typeName;
        public final String subtypeName;
        
        public DecodedPacket(int packetId, long timestamp, byte[] payload, boolean valid) {
            this.packetId = packetId;
            this.timestamp = timestamp;
            this.payload = payload;
            this.valid = valid;
            this.type = (packetId >> 2) & 0x3F;
            this.subtype = packetId & 0x03;
            this.typeName = PacketType.getName(this.type);
            this.subtypeName = PacketSubtype.getName(this.subtype);
        }
        
        /**
         * Create DecodedPacket from serial Packet with validity flag
         */
        public static DecodedPacket fromPacket(Packet packet, boolean valid) {
            return new DecodedPacket(
                packet.packetType & 0xFF,
                packet.timestamp,
                packet.payload != null ? packet.payload : new byte[0],
                valid
            );
        }
        
        @Override
        public String toString() {
            return String.format("<DecodedPacket id=%d ts=%d len=%d valid=%b type=%s subtype=%s>",
                packetId, timestamp, payload.length, valid, typeName, subtypeName);
        }
    }
    
    // CRC16 Functions - delegated to SerialFrameCRC
    
    /**
     * Update CRC16 with one byte using SerialFrameCRC table
     */
    public static int crc16Update(int crc, int b) {
        return SerialFrameCRC.getCrcTable()[(crc ^ b) & 0xFF] ^ (crc >> 8);
    }
    
    /**
     * Compute CRC16 over data buffer
     */
    public static int crc16Reflected(byte[] data) {
        return crc16Reflected(data, 0, data.length);
    }
    
    /**
     * Compute CRC16 over data buffer with offset and length
     */
    public static int crc16Reflected(byte[] data, int offset, int length) {
        int crc = 0xFFFF;
        for (int i = offset; i < offset + length; i++) {
            crc = crc16Update(crc, data[i] & 0xFF);
        }
        return (~crc) & 0xFFFF;
    }
    
    /**
     * Check if CRC is valid
     */
    public static boolean crcValid(int computed, int received) {
        return ((~computed) & 0xFFFF) == received;
    }
    
    /**
     * Create packet ID from type and subtype
     */
    public static int createPacketId(int pktType, int pktSubtype) {
        return (pktType << 2) | (pktSubtype & 0x03);
    }
    
    /**
     * Encode packet using SerialFrameBuilder
     */
    public static byte[] encodePacket(int packetId, int timestamp, byte[] payload) {
        if (payload.length > 255) {
            throw new IllegalArgumentException("Payload too large for protocol (" + payload.length + " > 255)");
        }
        
        SerialFrameBuilder builder = new SerialFrameBuilder();
        
        // Append packet ID
        builder.append((byte) packetId);
        
        // Append timestamp (little endian)
        builder.append((byte) (timestamp & 0xFF));
        builder.append((byte) ((timestamp >> 8) & 0xFF));
        
        // Append payload length
        builder.append((byte) payload.length);
        
        // Append payload
        builder.appendMany(payload);
        
        return builder.end();
    }
    
    /**
     * Helper to encode packet from type/subtype
     */
    public static byte[] encodeFrame(int pktType, int pktSubtype, byte[] payload) {
        return encodeFrame(pktType, pktSubtype, payload, 0);
    }
    
    /**
     * Helper to encode packet from type/subtype with timestamp
     */
    public static byte[] encodeFrame(int pktType, int pktSubtype, byte[] payload, int timestamp) {
        int pktId = createPacketId(pktType, pktSubtype);
        return encodePacket(pktId, timestamp, payload);
    }
    
    /**
     * Packet Decoder using SerialFrameParser from serial package
     */
    public static class PacketDecoder {
        private final SerialFrameParser parser = new SerialFrameParser();
        private int packetId = 0;
        private int timestamp = 0;
        private final ByteArrayOutputStream payload = new ByteArrayOutputStream();
        
        public void reset() {
            parser.reset();
            packetId = 0;
            timestamp = 0;
            payload.reset();
        }
        
        /**
         * Feed one byte to the decoder
         * @return DecodedPacket if a complete packet is decoded, null otherwise
         */
        public DecodedPacket feed(int b) {
            Byte processed = parser.process((byte) b);
            
            if (parser.result == null) {
                return null;
            }
            
            switch (parser.result) {
                case STX:
                    // Reset state on new packet
                    packetId = 0;
                    timestamp = 0;
                    payload.reset();
                    return null;
                    
                case PktID:
                    if (processed != null) {
                        packetId = processed & 0xFF;
                    }
                    return null;
                    
                case Timestamp_LSB:
                    if (processed != null) {
                        timestamp = processed & 0xFF;
                    }
                    return null;
                    
                case Timestamp_MSB:
                    if (processed != null) {
                        timestamp |= ((processed & 0xFF) << 8);
                    }
                    return null;
                    
                case PayloadLength:
                    // Length is handled internally by parser
                    return null;
                    
                case Payload:
                case Continue:
                    if (processed != null && parser.Expected == SerialFrameParser.Status.Payload) {
                        payload.write(processed & 0xFF);
                    } else if (processed != null && parser.result == SerialFrameParser.Status.Payload) {
                        payload.write(processed & 0xFF);
                    }
                    return null;
                    
                case CRC_LSB:
                    return null;
                    
                case CRC_MSB:
                    // Packet complete
                    boolean valid = !parser.fail;
                    DecodedPacket packet = new DecodedPacket(
                        packetId,
                        timestamp,
                        payload.toByteArray(),
                        valid
                    );
                    reset();
                    return packet;
                    
                case MismatchedCRC:
                    // CRC error - return invalid packet
                    DecodedPacket invalidPacket = new DecodedPacket(
                        packetId,
                        timestamp,
                        payload.toByteArray(),
                        false
                    );
                    reset();
                    return invalidPacket;
                    
                default:
                    return null;
            }
        }
    }
    
    /**
     * Incremental stream decoder. Feed incoming bytes to 'append' and pull out packets via 'popPackets'.
     */
    public static class PacketStream {
        private ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        
        /**
         * Append raw bytes to the internal buffer.
         */
        public void append(byte[] data) {
            buffer.write(data, 0, data.length);
        }
        
        /**
         * Append raw bytes to the internal buffer with offset and length.
         */
        public void append(byte[] data, int offset, int length) {
            buffer.write(data, offset, length);
        }
        
        /**
         * Decode as many packets as possible from the current buffer,
         * remove consumed bytes, and return the list of packets.
         */
        public List<DecodedPacket> popPackets() {
            List<DecodedPacket> packets = new ArrayList<>();
            byte[] bufferView = buffer.toByteArray();
            int bufLen = bufferView.length;
            int totalConsumed = 0;
            PacketDecoder decoder = new PacketDecoder();
            
            while (totalConsumed < bufLen) {
                // If we are at start and not STX, skip to next STX
                if ((bufferView[totalConsumed] & 0xFF) != (STX & 0xFF)) {
                    int nextStx = -1;
                    for (int i = totalConsumed; i < bufLen; i++) {
                        if ((bufferView[i] & 0xFF) == (STX & 0xFF)) {
                            nextStx = i;
                            break;
                        }
                    }
                    if (nextStx == -1) {
                        // No more STX, discard all remaining garbage
                        totalConsumed = bufLen;
                        break;
                    } else {
                        totalConsumed = nextStx;
                    }
                }
                
                // Now bufferView[totalConsumed] is STX
                // Try to decode one packet
                boolean packetFound = false;
                int currentIdx = totalConsumed;
                
                while (currentIdx < bufLen) {
                    int b = bufferView[currentIdx] & 0xFF;
                    DecodedPacket pkt = decoder.feed(b);
                    currentIdx++;
                    
                    if (pkt != null) {
                        packets.add(pkt);
                        packetFound = true;
                        totalConsumed = currentIdx;
                        break;
                    }
                }
                
                if (!packetFound) {
                    // We ran out of data without completing a packet.
                    break;
                }
            }
            
            // Remove consumed bytes
            if (totalConsumed > 0) {
                byte[] remaining = new byte[bufLen - totalConsumed];
                System.arraycopy(bufferView, totalConsumed, remaining, 0, remaining.length);
                buffer.reset();
                if (remaining.length > 0) {
                    buffer.write(remaining, 0, remaining.length);
                }
            }
            
            return packets;
        }
        
        /**
         * Clear the internal buffer.
         */
        public void clear() {
            buffer.reset();
        }
    }
}
