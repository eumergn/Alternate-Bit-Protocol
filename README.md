# Alternate-Bit-Protocol

This project is a simple and educational implementation of the **Alternating Bit Protocol (ABP)** using **UDP sockets in C**. It ensures reliable data transmission over an unreliable medium by using acknowledgments and retransmissions.

---

## 📁 Files

- `sender.c`   – Sends data to the receiver using the alternating bit protocol.
- `medium.c`   – Simulates an unreliable network that randomly drops packets.
- `receiver.c` – Receives and acknowledges data packets based on sequence numbers.

---

## Compilation
A `Makefile` is provided for convenient compilation of all three components.

### To build all programs:

```bash
make
```

### To compile a specific component:
```bash
make sender
make medium
make receiver
```
### To clean/remove all compiled binaries:
```bash
make clean
```


## Learning Objectives
- Understand how to implement reliable transmission over unreliable protocols like UDP.
- Practice socket programming using the POSIX API in C.
- Learn about sequence numbering, acknowledgments, timeouts, and retransmissions.
- Simulate packet loss and test network robustness.


## Usage
> Make sure to compile the programs first using make (see [Compilation](#compilation))

1. Run the Receiver
```bash
./receiver <local_port>
```
2. Run the Medium
```bash
./medium <local_port> <sender_ip> <sender_port> <receiver_ip> <receiver_port> <loss_rate>
```
3. Run the Sender
```bash
./sender <local_port> <medium_ip> <medium_port>
```

## How It Works
1. The Sender sends data with a sequence number (alternating between 0 and 1).
2. The Medium randomly drops messages to simulate an unreliable network.
3. The Receiver checks the sequence number and sends an ACK for correctly received messages.
4.The Sender waits for the correct ACK before sending the next message. If not received within a timeout or if incorrect, it retransmits.


