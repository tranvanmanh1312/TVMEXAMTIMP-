FROM debian:buster

LABEL maintainer="Dev Team <dev@example.com>"

ENV DEBIAN_FRONTEND=noninteractive

RUN set -eux; \
    printf "deb http://archive.debian.org/debian buster main\n\
deb http://archive.debian.org/debian-security buster/updates main\n\
deb http://deb.debian.org/debian bullseye main\n\
deb http://deb.debian.org/debian bookworm main\n" > /etc/apt/sources.list; \
    printf 'Acquire::Check-Valid-Until "false";\nAcquire::Retries "5";\n' > /etc/apt/apt.conf.d/99archive; \
    apt-get update; \
    apt-get install -y --no-install-recommends -t bullseye libcrypt1; \
    apt-get install -y --no-install-recommends -t bookworm \
        ca-certificates \
        g++ \
        make \
        qmake6 \
        qt6-base-dev \
        qt6-base-dev-tools; \
    rm -rf /var/lib/apt/lists/*

WORKDIR /opt/tcp_app

COPY tcp_app /opt/tcp_app

RUN qmake6 echoServer.pro "TARGET=server_app" && \
    make -j$(nproc) && \
    test -f /opt/tcp_app/server_app

EXPOSE 8080

CMD ["./server_app"]