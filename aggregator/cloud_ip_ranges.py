#!/usr/bin/python3
#referance: https://github.com/nccgroup/cloud_ip_ranges
#We edited and used this given repo to create our cloud providers data more efficiently
import requests
from netaddr import IPNetwork
from lxml import html
import csv

aws_url = 'https://ip-ranges.amazonaws.com/ip-ranges.json'
aws_ips = None

azure_url = 'https://www.microsoft.com/en-us/download/confirmation.aspx?id=56519'
azure_ips = None

gcp_url = 'https://www.gstatic.com/ipranges/cloud.json'
gcp_ips = None

oci_url = 'https://docs.cloud.oracle.com/en-us/iaas/tools/public_ip_ranges.json'
oci_ips = None

do_url = 'http://digitalocean.com/geo/google.csv'
do_ips = None

def init():
    global aws_ips, azure_ips, gcp_ips, oci_ips, do_ips
    aws_ips = requests.get(aws_url, allow_redirects=True).json()
    page = requests.get(azure_url)
    tree = html.fromstring(page.content)
    download_url = tree.xpath("//a[contains(@class, 'failoverLink') and contains(@href,'download.microsoft.com/download/')]/@href")[0]
    azure_ips = requests.get(download_url, allow_redirects=True).json()
    gcp_ips = requests.get(gcp_url, allow_redirects=True).json()
    oci_ips = requests.get(oci_url, allow_redirects=True).json()
    do_ips_request = requests.get(do_url, allow_redirects=True)
    do_ips = csv.DictReader(do_ips_request.content.decode('utf-8').splitlines(), fieldnames = ['range', 'country', 'region', 'city', 'postcode'])


def match_aws(target_ip):
    try:
        for item in aws_ips["prefixes"]:
            if target_ip in IPNetwork(str(item["ip_prefix"])):
                return {
                    "provider": "AWS",
                    "range": item["ip_prefix"],
                    "region": item["region"],
                    "service": item["service"],
                }

        for item in aws_ips["ipv6_prefixes"]:
            if target_ip in IPNetwork(str(item["ipv6_prefix"])):
                return {
                    "provider": "AWS",
                    "range": item["ipv6_prefix"],
                    "region": item["region"],
                    "service": item["service"],
                }

    except Exception as e:
        pass

    return None


def match_azure(target_ip):
    try:
        for item in azure_ips["values"]:
            for prefix in item["properties"]['addressPrefixes']:
                if target_ip in IPNetwork(str(prefix)):
                    return {
                        "provider": "Azure",
                        "range": prefix,
                        "region": item["properties"]["region"],
                        "service": item["properties"]["systemService"]
                    }

    except Exception as e:
        pass

    return None


def match_gcp(target_ip):
    try:
        for item in gcp_ips["prefixes"]:
            if target_ip in IPNetwork(str(item.get("ipv4Prefix", item.get("ipv6Prefix")))):
                return {
                    "provider": "GCP",
                    "range": item.get("ipv4Prefix", item.get("ipv6Prefix")),
                    "region": item["scope"],
                    "service": item["service"],
                }

    except Exception as e:
        pass

    return None


def match_oci(target_ip):
    try:
        for region in oci_ips["regions"]:
            for cidr_item in region['cidrs']:
                if target_ip in IPNetwork(str(cidr_item["cidr"])):
                    return {
                        "provider": "OCI",
                        "range": cidr_item["cidr"],
                        "region": region["region"],
                        "service": cidr_item["tags"][-1]
                    }

    except Exception as e:
        pass

    return None


def match_do(target_ip):
    try:
        for item in do_ips:
            if target_ip in IPNetwork(item['range']):
                return {
                    "provider": "DigitalOcean",
                    "range": item["range"],
                    "country": item["country"],
                    "region": item["region"],
                    "city": item["city"],
                    "postcode": item["postcode"]
                }

    except Exception as e:
        pass

    return None


def matching_providers(target_ip):
    match = match_aws(target_ip)
    if match != None:
        return match
    
    match = match_azure(target_ip)
    if match != None:
        return match
    
    match = match_gcp(target_ip)
    if match != None:
        return match
    
    match = match_oci(target_ip)
    if match != None:
        return match
    
    match = match_do(target_ip)
    if match != None:
        return match
    
    return None