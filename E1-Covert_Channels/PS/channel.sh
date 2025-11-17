targets=("error-rate" "sender" "receiver" "trace.txt" "decode_raw.txt" "send_bits.txt" "receive_bits.txt" "result.txt")

for target in "${targets[@]}"; do
    if [ -e "$target" ]; then
        rm -rf "$target"  
        echo "remove: $target"
    fi
done


gcc sender.c -o sender
g++ receiver.cpp -o receiver -lpthread

gcc error-rate.c -o error-rate

send_bits=1000
sampling_rate_us=10

for per_bit_us in {340..120..-20}; do
    echo per_bit_us: $per_bit_us us >> result.txt
    taskset -c 0 ./sender $send_bits $per_bit_us &
    (time ./receiver $send_bits $per_bit_us $sampling_rate_us) 2>> result.txt
    sync
    sleep 3
    ./error-rate $send_bits send_bits.txt receive_bits.txt >> result.txt
done

echo "finish!"
