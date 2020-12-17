## TITLE
VLC/Wi-Fi Hybrid Communication System    
## SPONSOR 
Xudong Wang, UM-SJTU Joint Institute    
## BACKGROUND
Visible light communication (VLC) has become a hot research topic due to its potential to support high data rates through ubiquitous LEDs. In the upcoming 5G network, VLC is also being considered as a viable option to alleviate “spectrum crunch”. So far many research groups have already demonstrated the capability of gigabit data transmission over VLC downlink.    
###
However, uplink design of such systems still remains an open problem. VLC uplink cannot coexist with VLC downlink properly for a few reasons. First, it is difficult for access points (APs) or base stations (BSs) to conduct detection and transmission at the same time, due to two factors: 1) signal power from an AP is usually much higher than that from a user; 2) VLC is intensity modulated, so the uplink and downlink signals cannot be distinguished in frequency. Second, because of the physical characteristics of light, VLC link quality can be easily deteriorated by blockage, ambient light, or even angle misalignment between transmitter and receiver, which highly constrains the mobility of users. Third, mobile terminals are usually power limited, which makes VLC unsuitable for uplink communications.

<p align="center">
  <img width="356" height="296" src="https://raw.githubusercontent.com/yuantong0407/VLC/main/photo.jpg" />
</p>

###
To resolve the uplink issue in VLC communications, we propose to integrate Wi-Fi with VLC. More specifically, in the hybrid wireless system, downlink communications are conducted via VLC, but uplink communications are leveraged through Wi-Fi. The key challenges of such a hybrid system are two-fold: 1) design of medium access control protocol to integrate VLC and Wi-Fi at packet level; 2) system integration of VLC and Wi-Fi into the same wireless link. The similar idea of VLC/Wi-Fi is mentioned in [1][2], but the above challenges have not been resolved yet.
## PURPOSE
In this capstone project, the students will participate in the design of VLC/Wi-Fi hybrid system on FPGA and Embedded PCs. They are expected to build a demo for such a hybrid system. This project will provide a comprehensive training for undergraduate students in multiple engineering disciplines. Through this project, students will acquire deep knowledge in communication systems, protocol architecture and design, embedded systems, and system integration. Students are encouraged to incorporate their creative ideas into the demo system.
## OUTCOME
Final outcome includes a demonstration of the VLC/Wi-Fi hybrid system where a point-to-point link can work properly at a reasonable setup of wireless LAN.
## References
[1] M. B. Rahaim and T. D. C. Little, “Toward Practical Integration of Dual-use VLC Within 5G Networks,” IEEE Wireless Communications, vol. 22, no. 4, pp. 97-103, Aug, 2015.
### 
[2] S. Shao, A. Khreishah, M. Ayyash, M. B. Rahaim, H. Elgala, and et al., “Design and Analysis of a Visible-Light-Communication Enhanced WiFi System," Journal of Optical Communications and Networking, vol. 7, no. 10, pp. 960-973, Oct, 2015.
###
[3] P. H. Pathak, X. Feng, P. Hu, and P. Mohapatra, “Visible Light Communication, Networking and Sensing: A Survey, Potential and Challenges,” IEEE Communications Surveys & Tutorials, vol. 17, no. 4, pp. 2047-2077, Sep, 2015.
