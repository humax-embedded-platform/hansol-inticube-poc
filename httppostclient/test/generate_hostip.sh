#!/bin/bash

# Function to generate IP addresses
generate_ip_addresses() {
    local count=$1
    local base_ip=$2
    local filename=$3

    if [[ ! $base_ip =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
        echo "Invalid base IP address format"
        exit 1
    fi

    # Extract the base IP octets
    IFS='.' read -r -a ip_parts <<< "$base_ip"

    local base_octet1=${ip_parts[0]}
    local base_octet2=${ip_parts[1]}
    local base_octet3=${ip_parts[2]}
    local base_octet4=${ip_parts[3]}

    # Remove the file if it already exists
    if [ -f "$filename" ]; then
        rm "$filename"
    fi

    # Create or clear the file
    touch "$filename"

    for ((i = 0; i < count; i++)); do
        local increment=$i

        # Calculate new octets with wrapping
        local octet4=$(( (base_octet4 + increment) % 256 ))
        local carry1=$(( (base_octet4 + increment) / 256 ))
        local octet3=$(( (base_octet3 + carry1) % 256 ))
        local carry2=$(( (base_octet3 + carry1) / 256 ))
        local octet2=$(( (base_octet2 + carry2) % 256 ))
        local carry3=$(( (base_octet2 + carry2) / 256 ))
        local octet1=$(( (base_octet1 + carry3) % 256 ))

        local ip_address="$octet1.$octet2.$octet3.$octet4"

        # Pad the IP address to ensure it is exactly 15 characters long
        local padded_ip=$(printf "%-15s" "$ip_address")

        echo "$padded_ip" >> "$filename"
    done

    echo "Host database generated: $filename"
}

# Main script
main() {
    local count=10000
    local base_ip="192.168.1.0"
    local filename="hostdb.txt"

    generate_ip_addresses $count $base_ip $filename
}

# Run the main function
main
