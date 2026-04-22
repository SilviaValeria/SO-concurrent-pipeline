FROM ubuntu:22.04

WORKDIR /app

COPY . .

RUN apt-get update && \
    apt-get install -y g++ make && \
    rm -rf /var/lib/apt/lists/*

RUN make build

CMD ["./pipeline"]
