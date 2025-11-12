#!/usr/bin/env python3
"""
VMG (Vehicle Mobile Gateway) Server Simulator
Accepts DoIP connection from Zonal Gateway and processes UDS commands
"""

import socket
import struct
import time
import threading

# DoIP Configuration
DOIP_PROTOCOL_VERSION = 0x02
DOIP_INVERSE_VERSION = 0xFD
DOIP_PAYLOAD_TYPE_ROUTING_ACT_REQ = 0x0005
DOIP_PAYLOAD_TYPE_ROUTING_ACT_RES = 0x0006
DOIP_PAYLOAD_TYPE_ALIVE_CHECK_REQ = 0x0007
DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RES = 0x0008
DOIP_PAYLOAD_TYPE_DIAG_MSG = 0x8001
DOIP_PAYLOAD_TYPE_DIAG_ACK = 0x8002
DOIP_PAYLOAD_TYPE_VCI_REPORT = 0x9000  # VCI Report from ZGW

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

class VMGServer:
    def __init__(self, host='0.0.0.0', port=13400):
        self.host = host
        self.port = port
        self.server_sock = None
        self.client_sock = None
        self.running = False
        
    def start(self):
        """Start VMG server"""
        print("="*60)
        print("VMG Server Simulator")
        print(f"Listening on {self.host}:{self.port}")
        print("Waiting for Zonal Gateway connection...")
        print("="*60)
        
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_sock.bind((self.host, self.port))
        self.server_sock.listen(1)
        
        self.running = True
        
        try:
            while self.running:
                print("\n[VMG] Waiting for connection...")
                self.client_sock, addr = self.server_sock.accept()
                print(f"[VMG] ✓ Connected from {addr[0]}:{addr[1]}")
                
                # Handle connection
                self.handle_client()
                
        except KeyboardInterrupt:
            print("\n[VMG] Shutting down...")
        finally:
            if self.client_sock:
                self.client_sock.close()
            if self.server_sock:
                self.server_sock.close()
                
    def handle_client(self):
        """Handle DoIP client (ZGW)"""
        try:
            while self.running:
                # Receive DoIP header (8 bytes)
                header = self.client_sock.recv(8)
                if not header or len(header) < 8:
                    print("[VMG] Connection closed by client")
                    break
                    
                ver, inv_ver, payload_type, payload_len = struct.unpack('>BBHL', header)
                
                # Receive payload
                payload = b''
                while len(payload) < payload_len:
                    chunk = self.client_sock.recv(payload_len - len(payload))
                    if not chunk:
                        break
                    payload += chunk
                    
                if len(payload) < payload_len:
                    print("[VMG] Incomplete payload")
                    break
                    
                # Process message
                self.process_message(payload_type, payload)
                
        except Exception as e:
            print(f"[VMG] Error: {e}")
        finally:
            if self.client_sock:
                self.client_sock.close()
                self.client_sock = None
                
    def process_message(self, payload_type, payload):
        """Process DoIP message"""
        if payload_type == DOIP_PAYLOAD_TYPE_ROUTING_ACT_REQ:
            print("\n[RX] Routing Activation Request")
            self.send_routing_activation_response()
            
        elif payload_type == DOIP_PAYLOAD_TYPE_ALIVE_CHECK_RES:
            print("[RX] Alive Check Response")
            
        elif payload_type == DOIP_PAYLOAD_TYPE_DIAG_MSG:
            print("\n[RX] Diagnostic Message")
            self.process_diagnostic_message(payload)
            
        elif payload_type == DOIP_PAYLOAD_TYPE_VCI_REPORT:
            print("\n" + "="*60)
            print("[RX] ✓ VCI REPORT RECEIVED FROM ZGW")
            print("="*60)
            self.process_vci_report(payload)
            
        else:
            print(f"[RX] Unknown message type: 0x{payload_type:04X}")
            
    def send_routing_activation_response(self):
        """Send Routing Activation Response"""
        # Response: SA(2) + TA(2) + ResponseCode(1) + Reserved(4)
        payload = struct.pack('>HHBI', ADDR_VMG, ADDR_ZGW, 0x10, 0)  # 0x10 = Success
        
        header = struct.pack('>BBHL',
                           DOIP_PROTOCOL_VERSION,
                           DOIP_INVERSE_VERSION,
                           DOIP_PAYLOAD_TYPE_ROUTING_ACT_RES,
                           len(payload))
        
        message = header + payload
        self.client_sock.sendall(message)
        print("[TX] Routing Activation Response (SUCCESS)")
        
    def process_diagnostic_message(self, payload):
        """Process UDS diagnostic message"""
        if len(payload) < 5:
            print("[VMG] Invalid diagnostic message")
            return
            
        # Parse routing
        sa = (payload[0] << 8) | payload[1]
        ta = (payload[2] << 8) | payload[3]
        uds_data = payload[4:]
        
        print(f"  Route: SA=0x{sa:04X}, TA=0x{ta:04X}")
        print(f"  UDS Data ({len(uds_data)} bytes): {' '.join(f'{b:02X}' for b in uds_data)}")
        
        # Parse UDS
        if len(uds_data) < 1:
            return
            
        sid = uds_data[0]
        
        # Decode UDS Service
        print(f"  Service ID: 0x{sid:02X}", end="")
        
        # Create response
        response_data = None
        
        if sid == UDS_SID_ROUTINE_CONTROL and len(uds_data) >= 4:
            print(" (Routine Control)")
            sub = uds_data[1]
            rid = (uds_data[2] << 8) | uds_data[3]
            
            print(f"    Sub-function: 0x{sub:02X}", end="")
            if sub == UDS_RC_START_ROUTINE:
                print(" (Start Routine)")
            else:
                print()
                
            print(f"    Routine ID: 0x{rid:04X}", end="")
            
            if sub == UDS_RC_START_ROUTINE:
                if rid == RID_VCI_COLLECTION_START:
                    print(" (VCI Collection Start)")
                    print("  ✓ Command: Start VCI Collection")
                    # Response: SID+0x40, sub, RID, status
                    response_data = bytes([sid + UDS_POSITIVE_RESPONSE, sub, 
                                         uds_data[2], uds_data[3], 0x00])
                    print("  → Sending: Positive Response (Status=0x00)")
                    
                elif rid == RID_VCI_SEND_REPORT:
                    print(" (VCI Send Report)")
                    print("  ✓ Command: Send VCI Report")
                    # Response: SID+0x40, sub, RID, status, count
                    response_data = bytes([sid + UDS_POSITIVE_RESPONSE, sub,
                                         uds_data[2], uds_data[3], 0x00, 0x02])
                    print("  → Sending: Positive Response (Status=0x00, Count=2)")
                else:
                    print(" (Unknown Routine)")
            else:
                print()
                
        elif sid == (UDS_SID_ROUTINE_CONTROL + UDS_POSITIVE_RESPONSE):
            print(" (Routine Control Response)")
            if len(uds_data) >= 4:
                sub = uds_data[1]
                rid = (uds_data[2] << 8) | uds_data[3]
                status = uds_data[4] if len(uds_data) > 4 else None
                
                print(f"    Sub-function: 0x{sub:02X}")
                print(f"    Routine ID: 0x{rid:04X}")
                if status is not None:
                    print(f"    Status: 0x{status:02X}", end="")
                    if status == 0x00:
                        print(" (Success)")
                    elif status == 0x01:
                        print(" (Not Connected)")
                    elif status == 0x02:
                        print(" (Send Error)")
                    else:
                        print()
                        
                if len(uds_data) > 5:
                    print(f"    Additional Data: {' '.join(f'{b:02X}' for b in uds_data[5:])}")
                    
        elif sid == 0x22:  # Read Data By Identifier
            print(" (Read Data By Identifier)")
            if len(uds_data) >= 3:
                did = (uds_data[1] << 8) | uds_data[2]
                print(f"    DID: 0x{did:04X}")
                
        elif sid == 0x62:  # Read Data By Identifier Response
            print(" (Read Data By Identifier Response)")
            if len(uds_data) >= 3:
                did = (uds_data[1] << 8) | uds_data[2]
                print(f"    DID: 0x{did:04X}")
                
                # Check if this is VCI data
                if did == 0xF195:  # Consolidated VCI
                    print("    → Consolidated VCI Data")
                    self.parse_vci_data(uds_data[3:])
                elif did == 0xF194:  # Individual VCI
                    print("    → Individual VCI Data")
                    self.parse_vci_data(uds_data[3:])
                    
        else:
            print(f" (Unknown/Other Service)")
            if len(uds_data) > 1:
                print(f"    Data: {' '.join(f'{b:02X}' for b in uds_data[1:])}")
        
        # Send response
        if response_data:
            self.send_diagnostic_response(ta, sa, response_data)
            
    def parse_vci_data(self, data):
        """Parse and display VCI data in human-readable format"""
        if len(data) < 1:
            return
            
        # First byte is VCI count
        vci_count = data[0]
        print(f"    VCI Count: {vci_count}")
        
        offset = 1
        vci_size = 48  # 16 + 8 + 8 + 16 = 48 bytes per VCI
        
        for i in range(vci_count):
            if offset + vci_size > len(data):
                print(f"    [VCI {i+1}] Incomplete data")
                break
                
            vci_data = data[offset:offset + vci_size]
            offset += vci_size
            
            # Parse VCI structure
            ecu_id = vci_data[0:16]
            sw_version = vci_data[16:24]
            hw_version = vci_data[24:32]
            serial_num = vci_data[32:48]
            
            # Convert to strings (remove null padding)
            ecu_id_str = ecu_id.decode('ascii', errors='ignore').rstrip('\x00')
            sw_ver_str = sw_version.decode('ascii', errors='ignore').rstrip('\x00')
            hw_ver_str = hw_version.decode('ascii', errors='ignore').rstrip('\x00')
            serial_str = serial_num.decode('ascii', errors='ignore').rstrip('\x00')
            
            print(f"\n    ╔═══ VCI Entry #{i+1} ═══════════════════════════════")
            print(f"    ║ ECU ID:      '{ecu_id_str}'")
            print(f"    ║ SW Version:  '{sw_ver_str}'")
            print(f"    ║ HW Version:  '{hw_ver_str}'")
            print(f"    ║ Serial Num:  '{serial_str}'")
            print(f"    ╚════════════════════════════════════════════════")
            
        if offset < len(data):
            remaining = data[offset:]
            print(f"    Remaining data: {' '.join(f'{b:02X}' for b in remaining)}")
            
    def process_vci_report(self, payload):
        """Process VCI Report message (0x9000)"""
        if len(payload) < 1:
            print("  [ERROR] Empty VCI report")
            return
            
        # First byte is VCI count
        vci_count = payload[0]
        print(f"\n  📊 Total ECUs: {vci_count}")
        print(f"  📦 Payload size: {len(payload)} bytes")
        
        # Parse VCI data
        self.parse_vci_data(payload)
        
        print("\n" + "="*60)
        print("✓ VCI Report processing complete")
        print("="*60)
            
    def send_diagnostic_response(self, sa, ta, uds_data):
        """Send UDS diagnostic response"""
        # DoIP routing + UDS data
        payload = struct.pack('>HH', sa, ta) + uds_data
        
        header = struct.pack('>BBHL',
                           DOIP_PROTOCOL_VERSION,
                           DOIP_INVERSE_VERSION,
                           DOIP_PAYLOAD_TYPE_DIAG_MSG,
                           len(payload))
        
        message = header + payload
        self.client_sock.sendall(message)
        print(f"[TX] Diagnostic Response: {' '.join(f'{b:02X}' for b in uds_data)}")
        
    def send_vci_collection_command(self):
        """Send VCI collection start command (for manual trigger)"""
        if not self.client_sock:
            print("[VMG] No active connection")
            return
            
        print("\n[VMG] Sending VCI Collection Start command...")
        
        # UDS: 0x31 01 F0 01
        uds_data = bytes([UDS_SID_ROUTINE_CONTROL, UDS_RC_START_ROUTINE,
                         (RID_VCI_COLLECTION_START >> 8) & 0xFF,
                         RID_VCI_COLLECTION_START & 0xFF])
        
        # DoIP routing + UDS
        payload = struct.pack('>HH', ADDR_VMG, ADDR_ZGW) + uds_data
        
        header = struct.pack('>BBHL',
                           DOIP_PROTOCOL_VERSION,
                           DOIP_INVERSE_VERSION,
                           DOIP_PAYLOAD_TYPE_DIAG_MSG,
                           len(payload))
        
        message = header + payload
        self.client_sock.sendall(message)
        print("[TX] VCI Collection Start command sent")


def main():
    server = VMGServer(host='0.0.0.0', port=13400)
    
    # Start server in thread
    server_thread = threading.Thread(target=server.start)
    server_thread.daemon = True
    server_thread.start()
    
    # Simple command interface
    print("\n" + "="*60)
    print("Commands:")
    print("  1 - Send VCI Collection Start")
    print("  2 - Send VCI Report Request")
    print("  q - Quit")
    print("="*60)
    
    try:
        while True:
            cmd = input("\nCommand: ").strip().lower()
            
            if cmd == 'q':
                break
            elif cmd == '1':
                if server.client_sock:
                    # Send VCI Collection Start
                    uds_data = bytes([UDS_SID_ROUTINE_CONTROL, UDS_RC_START_ROUTINE,
                                    (RID_VCI_COLLECTION_START >> 8) & 0xFF,
                                    RID_VCI_COLLECTION_START & 0xFF])
                    payload = struct.pack('>HH', ADDR_VMG, ADDR_ZGW) + uds_data
                    header = struct.pack('>BBHL', DOIP_PROTOCOL_VERSION,
                                       DOIP_INVERSE_VERSION,
                                       DOIP_PAYLOAD_TYPE_DIAG_MSG,
                                       len(payload))
                    server.client_sock.sendall(header + payload)
                    print("[TX] VCI Collection Start command sent")
                else:
                    print("[VMG] No active connection")
                    
            elif cmd == '2':
                if server.client_sock:
                    # Send VCI Report Request
                    uds_data = bytes([UDS_SID_ROUTINE_CONTROL, UDS_RC_START_ROUTINE,
                                    (RID_VCI_SEND_REPORT >> 8) & 0xFF,
                                    RID_VCI_SEND_REPORT & 0xFF])
                    payload = struct.pack('>HH', ADDR_VMG, ADDR_ZGW) + uds_data
                    header = struct.pack('>BBHL', DOIP_PROTOCOL_VERSION,
                                       DOIP_INVERSE_VERSION,
                                       DOIP_PAYLOAD_TYPE_DIAG_MSG,
                                       len(payload))
                    server.client_sock.sendall(header + payload)
                    print("[TX] VCI Report Request command sent")
                else:
                    print("[VMG] No active connection")
                    
    except KeyboardInterrupt:
        print("\n[VMG] Interrupted")
    finally:
        server.running = False


if __name__ == '__main__':
    main()

