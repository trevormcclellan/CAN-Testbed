# CANBench Automotive Security Testbed

CANBench is a versatile and accessible testbed designed to advance research in automotive cybersecurity. Modern vehicles rely heavily on Controller Area Network (CAN) communication to enable seamless interactions between Electronic Control Units (ECUs). However, the CAN protocol, developed in the 1980s, was not designed with security in mind, leaving it vulnerable to various cyber-attacks.

CANBench provides a low-cost, physical platform for researchers, educators, and students to simulate and analyze attacks and defenses on CAN networks. Built with off-the-shelf components, the testbed offers a controlled environment to study CAN bus security without the need for proprietary automotive systems. Its features include:

- Attack Simulation: Replay CAN traffic in near real-time and execute multiple types of simulated attacks to evaluate vulnerabilities and mitigation strategies.
- Reproducibility: Designed with simplicity in mind, CANBench ensures that users with minimal prior experience can set up the testbed and replicate experiments with ease.
- Educational Use: Comprehensive documentation makes CANBench ideal for classroom environments and hands-on learning, empowering users to explore automotive network security.

# Getting Started with CANBench

Getting started with CANBench is simple! Follow these steps to set up the testbed and begin your exploration of automotive network security. For detailed setup instructions and background information, visit the [CANBench Wiki](https://github.com/trevormcclellan/CAN-Testbed/wiki).  

## Quick Start Guide  

1. **Assemble the Hardware:**  
   Gather the required off-the-shelf components listed in the [Hardware Setup](https://github.com/trevormcclellan/CAN-Testbed/wiki/Hardware-Setup) section of the wiki. Follow the instructions to assemble the testbed.

2. **Prepare the Software Environment:**  
   Clone the CANBench repository and install the required dependencies as described in the [Software Setup](https://github.com/trevormcclellan/CAN-Testbed/wiki/Software-Setup) section.

   ```bash
   git clone https://github.com/trevormcclellan/CAN-Testbed.git
   cd CAN-Testbed
   ```

3. **Run the Testbed:**
    Launch the testbed application and begin simulating attacks or analyzing CAN traffic. Detailed instructions for running the software can be found in the [Running the Testbed](https://github.com/trevormcclellan/CAN-Testbed/wiki/Running-the-Testbed) section.

4. **Start Experimenting:**
    Explore the capabilities of CANBench by simulating attacks, implementing mitigations, or using it as a teaching tool. Check out example use cases and tutorials in the wiki to get inspired.

For questions or troubleshooting, consult the [Wiki](https://github.com/trevormcclellan/CAN-Testbed/wiki) or open an issue on the repository.