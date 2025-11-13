#!/usr/bin/env python3
"""
ECU_011 Simulator
Simulates Zone ECU that listens for VCI broadcast requests and responds
"""

import socket
import struct
import time
import sys
import threading

class ECU_011_Simulator:
    """ECU_011 Simulator - Listens for VCI and Readiness requests and responds"""
    
    # VCI Message Magic Numbers
    VCI_MAGIC = 0x56434921  # "VCI!"
    VCI_REQUEST_MAGIC = b'RQST'  # VCI collection request from ZGW
    READINESS_REQUEST_MAGIC = b'\xAA\xBB\x03\x00'  # Readiness check request from ZGW
    
    def __init__(self, listen_port=13400):
        self.listen_port = listen_port
        
        # ECU_011 VCI Information (matches doip_types.h)
        self.ecu_id = "ECU_011"
        self.sw_version = "0.0.0"
        self.hw_version = "0.0.0"
        self.serial_num = "011000001"
        
        # ECU_011 Readiness Information
        self.vehicle_parked = True
        self.engine_off = True
        self.battery_voltage_mv = 13200  # 13.2V
        self.available_memory_kb = 3072   # 3MB
        self.all_doors_closed = True
        self.compatible = True
        self.ready_for_update = True      # All conditions met
        
        # Create UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        
        self.running = False
        
    def create_vci_message(self):
        """Create VCI message payload"""
        # Message format:
        # Magic Number (4 bytes) + ECU ID (16) + SW Ver (8) + HW Ver (8) + Serial (16)
        
        message = struct.pack('!I', self.VCI_MAGIC)
        
        # Pad strings to fixed length
        ecu_id_bytes = self.ecu_id.encode('ascii').ljust(16, b'\x00')
        sw_ver_bytes = self.sw_version.encode('ascii').ljust(8, b'\x00')
        hw_ver_bytes = self.hw_version.encode('ascii').ljust(8, b'\x00')
        serial_bytes = self.serial_num.encode('ascii').ljust(16, b'\x00')
        
        message += ecu_id_bytes + sw_ver_bytes + hw_ver_bytes + serial_bytes
        
        return message
    
    def create_readiness_message(self):
        """Create Readiness message payload"""
        # OTA_ReadinessInfo structure:
        # ecu_id(16) + vehicle_parked(1) + engine_off(1) + battery_voltage_mv(2) +
        # available_memory_kb(4) + all_doors_closed(1) + compatible(1) + ready_for_update(1)
        # = 27 bytes
        
        message = b''
        
        # ECU ID (16 bytes)
        ecu_id_bytes = self.ecu_id.encode('ascii').ljust(16, b'\x00')
        message += ecu_id_bytes
        
        # Conditions (booleans as uint8, 0 or 1)
        message += bytes([1 if self.vehicle_parked else 0])
        message += bytes([1 if self.engine_off else 0])
        
        # Battery voltage (uint16, big-endian)
        message += struct.pack('>H', self.battery_voltage_mv)
        
        # Available memory (uint32, big-endian)
        message += struct.pack('>I', self.available_memory_kb)
        
        # More conditions
        message += bytes([1 if self.all_doors_closed else 0])
        message += bytes([1 if self.compatible else 0])
        message += bytes([1 if self.ready_for_update else 0])
        
        return message
    
    def send_readiness_response(self, zgw_address):
        """Send Readiness response to ZGW"""
        message = self.create_readiness_message()
        
        try:
            # Send to ZGW address
            self.sock.sendto(message, zgw_address)
            print(f"\n[ECU_011] Readiness Response sent to {zgw_address[0]}:{zgw_address[1]}")
            print(f"  ECU ID: {self.ecu_id}")
            print(f"  Vehicle Parked: {self.vehicle_parked}")
            print(f"  Engine Off: {self.engine_off}")
            print(f"  Battery: {self.battery_voltage_mv} mV ({self.battery_voltage_mv/1000:.2f}V)")
            print(f"  Available Memory: {self.available_memory_kb} KB ({self.available_memory_kb/1024:.2f}MB)")
            print(f"  All Doors Closed: {self.all_doors_closed}")
            print(f"  Compatible: {self.compatible}")
            print(f"  Ready for Update: {'YES' if self.ready_for_update else 'NO'}")
            return True
        except Exception as e:
            print(f"[ECU_011] Failed to send Readiness response: {e}")
            return False
    
    def send_vci_response(self, zgw_address):
        """Send VCI response to ZGW"""
        message = self.create_vci_message()
        
        try:
            # Send to ZGW address (should be 192.168.1.10:13400)
            self.sock.sendto(message, zgw_address)
            print(f"\n[ECU_011] VCI Response sent to {zgw_address[0]}:{zgw_address[1]}")
            print(f"  ECU ID: {self.ecu_id}")
            print(f"  SW Version: {self.sw_version}")
            print(f"  HW Version: {self.hw_version}")
            print(f"  Serial Number: {self.serial_num}")
            return True
        except Exception as e:
            print(f"[ECU_011] Failed to send VCI response: {e}")
            return False
    
    def listen_for_requests(self):
        """Listen for VCI collection requests from ZGW"""
        try:
            # Bind to all interfaces on UDP 13400
            self.sock.bind(('', self.listen_port))
            print(f"[ECU_011] Listening on UDP port {self.listen_port}")
            print("[ECU_011] Waiting for VCI collection requests...")
            
            self.running = True
            
            while self.running:
                try:
                    # Receive broadcast message
                    self.sock.settimeout(1.0)  # 1 second timeout for clean shutdown
                    data, addr = self.sock.recvfrom(1024)
                    
                    # Check if this is a VCI collection request
                    if data == self.VCI_REQUEST_MAGIC:
                        print(f"\n[ECU_011] ✓ VCI Request received from {addr[0]}:{addr[1]}")
                        
                        # Small delay to simulate processing time
                        time.sleep(0.1)
                        
                        # Send VCI response back to ZGW
                        zgw_address = (addr[0], self.listen_port)
                        self.send_vci_response(zgw_address)
                    
                    # Check if this is a Readiness check request
                    elif data == self.READINESS_REQUEST_MAGIC:
                        print(f"\n[ECU_011] ✓ Readiness Request received from {addr[0]}:{addr[1]}")
                        
                        # Small delay to simulate processing time
                        time.sleep(0.1)
                        
                        # Send Readiness response back to ZGW
                        zgw_address = (addr[0], self.listen_port)
                        self.send_readiness_response(zgw_address)
                        
                except socket.timeout:
                    continue  # Normal timeout, continue listening
                except Exception as e:
                    if self.running:
                        print(f"[ECU_011] Error receiving data: {e}")
                    
        except Exception as e:
            print(f"[ECU_011] Failed to bind socket: {e}")
        finally:
            self.sock.close()
    
    def run(self):
        """Run simulator - listen for requests"""
        print("=" * 60)
        print("ECU_011 Simulator Started")
        print(f"Mode: Listen for VCI and Readiness requests on UDP {self.listen_port}")
        print("Press Ctrl+C to stop")
        print("=" * 60)
        
        try:
            self.listen_for_requests()
        except KeyboardInterrupt:
            print("\n[ECU_011] Shutting down...")
            self.running = False


def main():
    """Main function"""
    # Default configuration
    listen_port = 13400
    
    # Parse command line arguments
    if len(sys.argv) > 1:
        listen_port = int(sys.argv[1])
    
    # Create and run simulator
    ecu = ECU_011_Simulator(listen_port)
    ecu.run()


if __name__ == '__main__':
    main()

