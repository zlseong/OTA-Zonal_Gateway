#!/usr/bin/env python3
"""
VCI Command Test Script for Zonal Gateway
Tests UDS 0x31 Routine Control commands for VCI management
"""

import socket
import struct
import time

# DoIP Configuration
DOIP_PROTOCOL_VERSION = 0x02
DOIP_INVERSE_VERSION = 0xFD
DOIP_PAYLOAD_TYPE_DIAG_MSG = 0x8001
DOIP_PAYLOAD_TYPE_DIAG_ACK = 0x8002
DOIP_PAYLOAD_TYPE_DIAG_NACK = 0x8003

# UDS Configuration
UDS_SID_ROUTINE_CONTROL = 0x31
UDS_RC_START_ROUTINE = 0x01
UDS_POSITIVE_RESPONSE = 0x40

# Routine IDs
RID_VCI_COLLECTION_START = 0xF001
RID_VCI_SEND_REPORT = 0xF002

# DoIP Addresses
ADDR_VMG = 0x0E00
ADDR_ZGW = 0x0100

class DoIPClient:
    def __init__(self, host='192.168.1.10', port=13400):
        self.host = host
        self.port = port
        self.sock = None
        
    def connect(self):
        """Connect to ZGW DoIP server"""
        print(f"[DoIP] Connecting to {self.host}:{self.port}...")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5.0)
        try:
            self.sock.connect((self.host, self.port))
            print("[DoIP] Connected!")
            return True
        except Exception as e:
            print(f"[DoIP] Connection failed: {e}")
            return False
            
    def disconnect(self):
        """Disconnect from DoIP server"""
        if self.sock:
            self.sock.close()
            self.sock = None
            print("[DoIP] Disconnected")
            
    def send_diagnostic_message(self, source_addr, target_addr, uds_data):
        """
        Send DoIP diagnostic message
        
        Args:
            source_addr: Source address (e.g., 0x0E00 for VMG)
            target_addr: Target address (e.g., 0x0100 for ZGW)
            uds_data: UDS message bytes (service ID + data)
        
        Returns:
            Response UDS data or None
        """
        # Build DoIP diagnostic message
        payload_length = 4 + len(uds_data)  # Routing (4) + UDS data
        
        # DoIP Header
        header = struct.pack('>BBHLL',
                           DOIP_PROTOCOL_VERSION,
                           DOIP_INVERSE_VERSION,
                           DOIP_PAYLOAD_TYPE_DIAG_MSG,
                           payload_length,
                           0)  # Padding to make 8 bytes
        header = header[:8]  # Truncate to 8 bytes
        
        # DoIP Routing
        routing = struct.pack('>HH', source_addr, target_addr)
        
        # Complete message
        message = header + routing + uds_data
        
        print(f"\n[TX] Sending {len(message)} bytes:")
        print(f"     DoIP: Ver={DOIP_PROTOCOL_VERSION:02X}, Type=0x{DOIP_PAYLOAD_TYPE_DIAG_MSG:04X}, Len={payload_length}")
        print(f"     Route: SA=0x{source_addr:04X}, TA=0x{target_addr:04X}")
        print(f"     UDS: {' '.join(f'{b:02X}' for b in uds_data)}")
        
        try:
            self.sock.sendall(message)
            
            # Receive response
            response_header = self.sock.recv(8)
            if len(response_header) < 8:
                print("[RX] Incomplete header received")
                return None
                
            ver, inv_ver, payload_type, payload_len = struct.unpack('>BBHL', response_header)
            
            print(f"\n[RX] Response header:")
            print(f"     DoIP: Ver={ver:02X}, Type=0x{payload_type:04X}, Len={payload_len}")
            
            # Receive payload
            payload = b''
            while len(payload) < payload_len:
                chunk = self.sock.recv(payload_len - len(payload))
                if not chunk:
                    break
                payload += chunk
                
            if len(payload) < payload_len:
                print(f"[RX] Incomplete payload (got {len(payload)}/{payload_len} bytes)")
                return None
                
            # Parse routing
            if len(payload) < 4:
                print("[RX] Payload too short")
                return None
                
            resp_sa, resp_ta = struct.unpack('>HH', payload[:4])
            uds_response = payload[4:]
            
            print(f"     Route: SA=0x{resp_sa:04X}, TA=0x{resp_ta:04X}")
            print(f"     UDS: {' '.join(f'{b:02X}' for b in uds_response)}")
            
            return uds_response
            
        except socket.timeout:
            print("[RX] Timeout waiting for response")
            return None
        except Exception as e:
            print(f"[RX] Error: {e}")
            return None
            
    def send_vci_collection_start(self):
        """Send UDS 0x31 01 F001 - Start VCI Collection"""
        print("\n" + "="*60)
        print("Command: Start VCI Collection")
        print("="*60)
        
        # UDS: 0x31 (Routine Control) + 0x01 (Start) + 0xF001 (RID)
        uds_data = bytes([UDS_SID_ROUTINE_CONTROL, 
                         UDS_RC_START_ROUTINE,
                         (RID_VCI_COLLECTION_START >> 8) & 0xFF,
                         RID_VCI_COLLECTION_START & 0xFF])
        
        response = self.send_diagnostic_message(ADDR_VMG, ADDR_ZGW, uds_data)
        
        if response and len(response) >= 4:
            sid = response[0]
            sub = response[1]
            rid = (response[2] << 8) | response[3]
            status = response[4] if len(response) > 4 else None
            
            if sid == (UDS_SID_ROUTINE_CONTROL + UDS_POSITIVE_RESPONSE):
                print(f"✓ SUCCESS: VCI collection started")
                if status is not None:
                    print(f"  Status code: 0x{status:02X}")
                return True
            else:
                print(f"✗ NEGATIVE RESPONSE: NRC=0x{response[1]:02X}")
                return False
        else:
            print("✗ FAILED: No valid response")
            return False
            
    def send_vci_report_request(self):
        """Send UDS 0x31 01 F002 - Send VCI Report"""
        print("\n" + "="*60)
        print("Command: Send VCI Report")
        print("="*60)
        
        # UDS: 0x31 (Routine Control) + 0x01 (Start) + 0xF002 (RID)
        uds_data = bytes([UDS_SID_ROUTINE_CONTROL,
                         UDS_RC_START_ROUTINE,
                         (RID_VCI_SEND_REPORT >> 8) & 0xFF,
                         RID_VCI_SEND_REPORT & 0xFF])
        
        response = self.send_diagnostic_message(ADDR_VMG, ADDR_ZGW, uds_data)
        
        if response and len(response) >= 4:
            sid = response[0]
            sub = response[1]
            rid = (response[2] << 8) | response[3]
            status = response[4] if len(response) > 4 else None
            ecu_count = response[5] if len(response) > 5 else None
            
            if sid == (UDS_SID_ROUTINE_CONTROL + UDS_POSITIVE_RESPONSE):
                print(f"✓ SUCCESS: VCI report sent")
                if status is not None:
                    print(f"  Status code: 0x{status:02X}")
                if ecu_count is not None:
                    print(f"  ECU count: {ecu_count}")
                return True
            else:
                print(f"✗ NEGATIVE RESPONSE: NRC=0x{response[1]:02X}")
                return False
        else:
            print("✗ FAILED: No valid response")
            return False

def main():
    print("="*60)
    print("VCI Command Test Script")
    print("Testing UDS 0x31 Routine Control for VCI Management")
    print("="*60)
    
    client = DoIPClient(host='192.168.1.10', port=13400)
    
    if not client.connect():
        print("\nFailed to connect to ZGW. Is it running?")
        return
        
    try:
        print("\nWaiting 2 seconds for DoIP activation...")
        time.sleep(2)
        
        # Test 1: Start VCI Collection
        print("\n[TEST 1] Start VCI Collection")
        client.send_vci_collection_start()
        
        # Wait for Zone ECUs to respond (simulate)
        print("\nWaiting 3 seconds for Zone ECUs to send VCI via UDP...")
        time.sleep(3)
        
        # Test 2: Send VCI Report
        print("\n[TEST 2] Send VCI Report")
        client.send_vci_report_request()
        
        print("\n" + "="*60)
        print("Test completed!")
        print("="*60)
        
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
    finally:
        client.disconnect()

if __name__ == '__main__':
    main()

