#include <iostream>
#include <vector>
#include <unordered_set>
#include <fstream>
#include "stdlib.h"
#include "SystemUtils.h"
#include "Packet.h"
#include "EthLayer.h"

#include "IPv4Layer.h"
#include "IPv6Layer.h"
#include "ArpLayer.h"
#include "IcmpLayer.h"

#include "TcpLayer.h"
#include "UdpLayer.h"

#include "HttpLayer.h"
#include "SSLLayer.h"
#include "SSHLayer.h"

#include "DnsLayer.h"

#include "PcapFileDevice.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
	std::string inputFile1;
	std::string outputFile;

	if (argc > 2) {
		inputFile1 = std::string(argv[1]);
		outputFile = std::string(argv[2]);

		std::cout << inputFile1 << " " << outputFile << std::endl;
	}
	else {
		std::cerr << "Wrong arguments" << std::endl;
		return 1;
	}

	std::fstream out_file;
	out_file.open(outputFile, std::ios_base::out);

	pcpp::IFileReaderDevice *reader = pcpp::IFileReaderDevice::getReader(inputFile1);
	if (reader == NULL) {
		std::cerr << "Cannot determine reader for file type" << std::endl;
		return 1;
	}

	if (!reader->open()) {
		std::cerr << "Cannot open for reading" << std::endl;
		return 1;
	}

	if (!out_file.is_open()) {
		std::cerr << "Cannot open for writing" << std::endl;
		return 1;
	}

	pcpp::RawPacket rawPacket;
	while (reader->getNextPacket(rawPacket)) {
		json j;
		std::vector<std::string> protocols;

		j["ts"] = rawPacket.getPacketTimeStamp().tv_sec * 1000000000 + rawPacket.getPacketTimeStamp().tv_nsec;

		pcpp::Packet parsedPacket(&rawPacket);
		pcpp::EthLayer *ethLayer = parsedPacket.getLayerOfType<pcpp::EthLayer>();

		j["srcMac"] = ethLayer->getSourceMac().toString();
		j["dstMac"] = ethLayer->getDestMac().toString();		

		pcpp::IPv4Layer *ipv4 = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
		pcpp::IPv6Layer *ipv6 = parsedPacket.getLayerOfType<pcpp::IPv6Layer>();

		pcpp::ArpLayer *arp = parsedPacket.getLayerOfType<pcpp::ArpLayer>();
		pcpp::IcmpLayer *icmp = parsedPacket.getLayerOfType<pcpp::IcmpLayer>();

		pcpp::TcpLayer *tcp = parsedPacket.getLayerOfType<pcpp::TcpLayer>();
		pcpp::UdpLayer *udp = parsedPacket.getLayerOfType<pcpp::UdpLayer>();

		pcpp::HttpRequestLayer *httpReq = parsedPacket.getLayerOfType<pcpp::HttpRequestLayer>();
		pcpp::HttpResponseLayer *httpRes = parsedPacket.getLayerOfType<pcpp::HttpResponseLayer>();

		pcpp::DnsLayer *dns = parsedPacket.getLayerOfType<pcpp::DnsLayer>();
		pcpp::SSLLayer *ssl = parsedPacket.getLayerOfType<pcpp::SSLLayer>();
		pcpp::SSHLayer *ssh = parsedPacket.getLayerOfType<pcpp::SSHLayer>();

		if (udp) {
			protocols.push_back("UDP");
			j["srcPort"] = udp->getSrcPort();
			j["dstPort"] = udp->getDstPort();
		}

		if (ipv4) {
			protocols.push_back("IPv4");
			j["srcIP"] = ipv4->getSrcIPAddress().toString();
			j["dstIP"] = ipv4->getDstIPAddress().toString();
		}
		
		if (ipv6) {
			protocols.push_back("IPv6");
			j["srcIP"] = ipv6->getSrcIPAddress().toString();
			j["dstIP"] = ipv6->getDstIPAddress().toString();
		}
		
		if (arp) {
			protocols.push_back("ARP");
		}

		if (icmp) {
			protocols.push_back("ICMP");
		}

		if (tcp) {
			protocols.push_back("TCP");
			j["srcPort"] = tcp->getSrcPort();
			j["dstPort"] = tcp->getDstPort();
		}

		if (httpReq || httpRes) {
			protocols.push_back("HTTP");
		}

		if (ssh) {
			protocols.push_back("SSH");
		}

		if (ssl) {
			protocols.push_back("SSL");
		}

		if (dns) {
			protocols.push_back("DNS");

			std::unordered_set<std::string> domains;
			pcpp::DnsQuery* query = dns->getFirstQuery();
			while (query) {
				domains.insert(query->getName());
				query = dns->getNextQuery(query);
			}

			pcpp::DnsResource* answer = dns->getFirstAnswer();
			while (answer) {
				domains.insert(answer->getName());
				answer = dns->getNextAnswer(answer);
			}

			j["domains"] = domains;
		}

		j["protocols"] = protocols;

		out_file << j.dump() << std::endl;
	}
	reader->close();

	out_file.close();
	return 0;
}