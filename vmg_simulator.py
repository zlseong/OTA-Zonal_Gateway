#!/usr/bin/env python3
"""
VMG (Vehicle Master Gateway) Simulator
DoIP Server implementation for testing Zonal Gateway
"""

import socket
import struct
import threading
import time
import sys
from datetime import datetime

# ============================================================================
# DoIP Message Class
# ============================================================================

class DoIPMessage:
    """DoIP Protocol Message Handler"""
    
    # Protocol Version
    PROTOCOL_VERSION = 0x02
    INVERSE_VERSION = 0xFD
    
    # Payload Types (ISO 13400-2)
    GENERIC_DOIP_HEADER_NACK = 0x0000
    VEHICLE_IDENTIFICATION_REQ = 0x0001
    VEHICLE_IDENTIFICATION_REQ_EID = 0x0002
    VEHICLE_IDENTIFICATION_REQ_VIN = 0x0003
    VEHICLE_IDENTIFICATION_RES = 0x0004
    ROUTING_ACTIVATION_REQ = 0x0005
    ROUTING_ACTIVATION_RES = 0x0006
    ALIVE_CHECK_REQ = 0x0007
    ALIVE_CHECK_RES = 0x0008
    DIAGNOSTIC_MESSAGE = 0x8001
    DIAGNOSTIC_MESSAGE_ACK = 0x8002
    DIAGNOSTIC_MESSAGE_NACK = 0x8003
    
    # Custom payload types
    VCI_REPORT = 0x9000
    HEALTH_STATUS_REPORT = 0x9001  # ECU Health Status (Dynamic monitoring)
    
    def __init__(self, payload_type, payload_data=b''):
        self.protocol_version = self.PROTOCOL_VERSION
        self.inverse_protocol_version = self.INVERSE_VERSION
        self.payload_type = payload_type
        self.payload_data = payload_data
        self.payload_length = len(payload_data)
    
    def to_bytes(self):
        """Serialize DoIP message to bytes"""
        header = struct.pack(
            '!BBHI',  # Network byte order (big-endian)
            self.protocol_version,
            self.inverse_protocol_version,
            self.payload_type,
            self.payload_length
        )
        return header + self.payload_data
    
    @staticmethod
    def from_bytes(data):
        """Deserialize bytes to DoIP message"""
        if len(data) < 8:
            raise ValueError(f"Invalid DoIP message: too short ({len(data)} bytes)")
        
        header = struct.unpack('!BBHI', data[:8])
        protocol_version = header[0]
        inverse_version = header[1]
        payload_type = header[2]
        payload_length = header[3]
        
        # Validate protocol version
        if protocol_version != DoIPMessage.PROTOCOL_VERSION:
            raise ValueError(f"Invalid protocol version: {protocol_version:#04x}")
        
        if inverse_version != DoIPMessage.INVERSE_VERSION:
            raise ValueError(f"Invalid inverse version: {inverse_version:#04x}")
        
        # Ensure we have the complete payload
        if len(data) < 8 + payload_length:
            raise ValueError(f"Incomplete payload: expected {payload_length}, got {len(data)-8}")
        
        payload_data = data[8:8+payload_length]
        
        return DoIPMessage(payload_type, payload_data)
    
    def get_payload_type_name(self):
        """Get human-readable payload type name"""
        type_names = {
            0x0000: "Generic NACK",
            0x0001: "Vehicle Identification Request",
            0x0002: "Vehicle Identification Request (EID)",
            0x0003: "Vehicle Identification Request (VIN)",
            0x0004: "Vehicle Identification Response",
            0x0005: "Routing Activation Request",
            0x0006: "Routing Activation Response",
            0x0007: "Alive Check Request",
            0x0008: "Alive Check Response",
            0x8001: "Diagnostic Message",
            0x8002: "Diagnostic Message ACK",
            0x8003: "Diagnostic Message NACK",
            0x9000: "VCI Report",
            0x9001: "Health Status Report",
        }
        return type_names.get(self.payload_type, f"Unknown ({self.payload_type:#06x})")


# ============================================================================
# VMG Simulator Class
# ============================================================================

class VMGSimulator:
    """Vehicle Master Gateway Simulator"""
    
    def __init__(self, host='0.0.0.0', port=13400):
        self.host = host
        self.port = port
        self.server_socket = None
        self.client_socket = None
        self.client_address = None
        self.running = False
        self.alive_check_thread = None
        
        # VMG Configuration
        self.vin = "TC375ZONALGW2024"  # 17 characters
        self.logical_address = 0x0100
        self.eid = bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66])
        self.gid = bytes([0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF])
        
        # Routing activation state
        self.routing_active = False
        self.source_address = None
    
    def log(self, message, level="INFO"):
        """Print timestamped log message"""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        prefix = {
            "INFO": "📡",
            "SUCCESS": "✅",
            "WARNING": "⚠️",
            "ERROR": "❌",
            "DEBUG": "🔍"
        }.get(level, "ℹ️")
        print(f"[{timestamp}] {prefix} {message}")
    
    def start(self):
        """Start VMG Simulator"""
        self.log(f"VMG Simulator starting on {self.host}:{self.port}", "INFO")
        
        try:
            # Create TCP server socket
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(1)
            self.server_socket.settimeout(1.0)  # For Ctrl+C responsiveness
            
            self.running = True
            self.log(f"VMG Simulator started successfully", "SUCCESS")
            self.log(f"Waiting for Zonal Gateway connection...", "INFO")
            
            # Main server loop
            while self.running:
                try:
                    client, addr = self.server_socket.accept()
                    self.client_socket = client
                    self.client_address = addr
                    self.log(f"Zonal Gateway connected from {addr[0]}:{addr[1]}", "SUCCESS")
                    
                    # Handle client connection
                    self.handle_client()
                    
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        self.log(f"Connection error: {e}", "ERROR")
        
        except KeyboardInterrupt:
            self.log("Received Ctrl+C, shutting down...", "WARNING")
        except Exception as e:
            self.log(f"Server error: {e}", "ERROR")
        finally:
            self.stop()
    
    def stop(self):
        """Stop VMG Simulator"""
        self.running = False
        
        if self.alive_check_thread and self.alive_check_thread.is_alive():
            self.alive_check_thread.join(timeout=2)
        
        if self.client_socket:
            try:
                self.client_socket.close()
            except:
                pass
        
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
        
        self.log("VMG Simulator stopped", "INFO")
    
    def handle_client(self):
        """Handle connected Zonal Gateway client"""
        try:
            while self.running:
                # Receive DoIP message
                header_data = self.recv_exact(8)
                if not header_data:
                    break
                
                # Parse header to get payload length
                _, _, payload_type, payload_length = struct.unpack('!BBHI', header_data)
                
                # Receive payload
                payload_data = self.recv_exact(payload_length)
                if not payload_data:
                    break
                
                # Parse complete message
                full_message = header_data + payload_data
                msg = DoIPMessage.from_bytes(full_message)
                
                self.log(f"← Received: {msg.get_payload_type_name()} ({len(full_message)} bytes)", "DEBUG")
                
                # Handle message based on type
                self.handle_message(msg)
        
        except Exception as e:
            self.log(f"Client handler error: {e}", "ERROR")
        finally:
            self.log(f"Zonal Gateway disconnected", "WARNING")
            self.routing_active = False
            if self.client_socket:
                try:
                    self.client_socket.close()
                except:
                    pass
            self.client_socket = None
    
    def recv_exact(self, num_bytes):
        """Receive exactly num_bytes from socket"""
        data = b''
        while len(data) < num_bytes:
            chunk = self.client_socket.recv(num_bytes - len(data))
            if not chunk:
                return None
            data += chunk
        return data
    
    def handle_message(self, msg):
        """Handle received DoIP message"""
        
        if msg.payload_type == DoIPMessage.ROUTING_ACTIVATION_REQ:
            self.handle_routing_activation(msg)
        
        elif msg.payload_type == DoIPMessage.ALIVE_CHECK_RES:
            self.handle_alive_check_response(msg)
        
        elif msg.payload_type == DoIPMessage.VCI_REPORT:
            self.handle_vci_report(msg)
        
        elif msg.payload_type == DoIPMessage.HEALTH_STATUS_REPORT:
            self.handle_health_status_report(msg)
        
        elif msg.payload_type == DoIPMessage.DIAGNOSTIC_MESSAGE:
            self.handle_diagnostic_message(msg)
        
        else:
            self.log(f"Unhandled message type: {msg.get_payload_type_name()}", "WARNING")
    
    def handle_routing_activation(self, msg):
        """Handle Routing Activation Request"""
        if len(msg.payload_data) < 7:
            self.log("Invalid Routing Activation Request", "ERROR")
            return
        
        # Parse request
        source_address = struct.unpack('!H', msg.payload_data[0:2])[0]
        activation_type = msg.payload_data[2]
        
        self.log(f"Routing Activation Request: Source=0x{source_address:04x}, Type={activation_type}", "INFO")
        
        # Store source address
        self.source_address = source_address
        
        # Create Routing Activation Response
        # Format: Source Address (2) + Target Address (2) + Response Code (1) + Reserved (4)
        response_payload = struct.pack(
            '!HHBL',
            self.logical_address,  # VMG logical address
            source_address,        # Client's source address
            0x10,                  # Response code: 0x10 = Success
            0x00000000            # Reserved
        )
        
        response = DoIPMessage(DoIPMessage.ROUTING_ACTIVATION_RES, response_payload)
        self.send_message(response)
        
        self.routing_active = True
        self.log("Routing Activation successful - starting Alive Check", "SUCCESS")
        
        # Start Alive Check thread
        self.start_alive_check()
    
    def handle_alive_check_response(self, msg):
        """Handle Alive Check Response"""
        if len(msg.payload_data) >= 2:
            source_address = struct.unpack('!H', msg.payload_data[0:2])[0]
            self.log(f"Alive Check Response from 0x{source_address:04x}", "DEBUG")
        else:
            self.log("Alive Check Response received", "DEBUG")
    
    def handle_vci_report(self, msg):
        """Handle VCI (Vehicle Configuration Information) Report"""
        if len(msg.payload_data) < 1:
            self.log("Invalid VCI Report", "ERROR")
            return
        
        self.log("=" * 60, "INFO")
        self.log("VCI REPORT RECEIVED", "SUCCESS")
        
        # Parse VCI report
        offset = 0
        ecu_count = msg.payload_data[offset]
        offset += 1
        
        self.log(f"Number of ECUs: {ecu_count}", "INFO")
        
        # Each ECU VCI: 48 bytes (ECU ID: 16, SW Ver: 8, HW Ver: 8, Serial: 16)
        for i in range(ecu_count):
            if offset + 48 > len(msg.payload_data):
                self.log(f"  ECU {i+1}: Incomplete data", "WARNING")
                break
            
            try:
                ecu_id = msg.payload_data[offset:offset+16].decode('ascii').rstrip('\x00')
                sw_version = msg.payload_data[offset+16:offset+24].decode('ascii').rstrip('\x00')
                hw_version = msg.payload_data[offset+24:offset+32].decode('ascii').rstrip('\x00')
                serial_num = msg.payload_data[offset+32:offset+48].decode('ascii').rstrip('\x00')
                offset += 48
                
                self.log(f"  ECU #{i+1}: {ecu_id}", "INFO")
                self.log(f"    SW Version: {sw_version}", "INFO")
                self.log(f"    HW Version: {hw_version}", "INFO")
                self.log(f"    Serial Number: {serial_num}", "INFO")
            except Exception as e:
                self.log(f"  ECU {i+1}: Parse error - {e}", "ERROR")
        
        self.log("=" * 60, "INFO")
    
    def handle_health_status_report(self, msg):
        """Handle ECU Health Status Report from Zonal Gateway"""
        if len(msg.payload_data) < 1:
            self.log("Invalid Health Status Report", "ERROR")
            return
        
        self.log("=" * 60, "INFO")
        self.log("ECU HEALTH STATUS REPORT RECEIVED", "SUCCESS")
        
        # Parse report
        offset = 0
        ecu_count = msg.payload_data[offset]
        offset += 1
        
        self.log(f"Number of ECUs: {ecu_count}", "INFO")
        
        # Each ECU Health: 24 bytes (ECU ID: 16, health: 1, DTC: 1, voltage: 2, temp: 1, reserved: 3)
        for i in range(ecu_count):
            if offset + 24 > len(msg.payload_data):
                self.log(f"  ECU {i+1}: Incomplete data", "WARNING")
                break
            
            try:
                ecu_id = msg.payload_data[offset:offset+16].decode('ascii').rstrip('\x00')
                health_status = msg.payload_data[offset + 16]
                dtc_count = msg.payload_data[offset + 17]
                battery_voltage = struct.unpack('!H', msg.payload_data[offset+18:offset+20])[0]  # Big-endian uint16
                temperature = msg.payload_data[offset + 20]
                offset += 24
                
                health_names = {0: "OK", 1: "WARNING", 2: "ERROR", 3: "CRITICAL"}
                
                self.log(f"  ECU #{i+1}: {ecu_id}", "INFO")
                self.log(f"    Status: {health_names.get(health_status, 'Unknown')}", "INFO")
                self.log(f"    DTCs: {dtc_count}", "INFO")
                self.log(f"    Battery: {battery_voltage/1000:.2f}V", "INFO")
                self.log(f"    Temperature: {temperature-40}°C", "INFO")
            except Exception as e:
                self.log(f"  ECU {i+1}: Parse error - {e}", "ERROR")
        
        self.log("=" * 60, "INFO")
    
    def handle_diagnostic_message(self, msg):
        """Handle Diagnostic Message (UDS)"""
        if len(msg.payload_data) < 5:
            self.log("Invalid Diagnostic Message", "ERROR")
            return
        
        source_addr = struct.unpack('!H', msg.payload_data[0:2])[0]
        target_addr = struct.unpack('!H', msg.payload_data[2:4])[0]
        uds_data = msg.payload_data[4:]
        
        self.log(f"Diagnostic Message: 0x{source_addr:04x} → 0x{target_addr:04x}, Data: {uds_data.hex()}", "INFO")
    
    def send_message(self, msg):
        """Send DoIP message to client"""
        if not self.client_socket:
            return
        
        try:
            data = msg.to_bytes()
            self.client_socket.send(data)
            self.log(f"→ Sent: {msg.get_payload_type_name()} ({len(data)} bytes)", "DEBUG")
        except Exception as e:
            self.log(f"Send error: {e}", "ERROR")
    
    def start_alive_check(self):
        """Start Alive Check periodic thread"""
        if self.alive_check_thread and self.alive_check_thread.is_alive():
            return
        
        self.alive_check_thread = threading.Thread(target=self.alive_check_loop, daemon=True)
        self.alive_check_thread.start()
    
    def alive_check_loop(self):
        """Periodic Alive Check sender"""
        while self.running and self.routing_active and self.client_socket:
            try:
                time.sleep(5.0)  # Send every 5 seconds
                
                if not self.routing_active:
                    break
                
                # Send Alive Check Request
                # Format: Source Address (2 bytes)
                payload = struct.pack('!H', self.logical_address)
                msg = DoIPMessage(DoIPMessage.ALIVE_CHECK_REQ, payload)
                self.send_message(msg)
                
            except Exception as e:
                self.log(f"Alive Check error: {e}", "ERROR")
                break


# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    print("=" * 60)
    print("  VMG (Vehicle Master Gateway) Simulator")
    print("  DoIP Server for Zonal Gateway Testing")
    print("=" * 60)
    print()
    
    simulator = VMGSimulator(host='0.0.0.0', port=13400)
    
    try:
        simulator.start()
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        simulator.stop()


if __name__ == "__main__":
    main()

