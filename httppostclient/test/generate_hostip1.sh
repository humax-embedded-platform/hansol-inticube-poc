#!/bin/bash

# Function to generate IP address with port
generate_ip_addresses() {
    local count=$1
    local ip=$2
    local filename=$3

    # Remove the file if it already exists
    if [ -f "$filename" ]; then
        rm "$filename"
    fi

    # Create or clear the file
    touch "$filename"

    for ((i = 0; i < count; i++)); do
        local port=$((9000 + (i % 10)))
        local ip_with_port="$ip:$port"

        # Pad the IP address with port to ensure it is exactly 24 characters long
        local padded_ip=$(printf "%-24s" "$ip_with_port")

        echo "$padded_ip" >> "$filename"
    done

    echo "Host database generated: $filename"
}

# Main script
main() {
    local count=100000
    local ip="192.168.100.237"
    local filename="hostdb.txt"

    generate_ip_addresses $count $ip $filename
}

# Run the main function
main
