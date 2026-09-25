"""Download only datasets/valid.parquet (5.4 GB) out of the 33.7 GB starter pack zip.

usage: python fetch_valid.py [OUT.parquet] [--limit MB]

The zip stores valid.parquet as one deflate stream at a known byte range, so
an HTTP Range request + raw inflate gives the exact file (size/CRC checked).
--limit stops after MB megabytes of compressed input (for testing only:
the parquet footer is at the end, so a partial file is not readable).
"""
import argparse
import sys
import urllib.request
import zlib

URL = 'https://files.wundernn.io/wnn_connectome_starterpack.zip'
START, END = 28345277158, 33777744739  # deflate data of valid.parquet, inclusive
SIZE, CRC = 5615509128, 0x3a65feaf


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out', nargs='?', default='valid.parquet')
    ap.add_argument('--limit', type=float, default=0)
    args = ap.parse_args()
    # (Cloudflare answers 403 to the default Python-urllib user agent)
    req = urllib.request.Request(URL, headers={'Range': f'bytes={START}-{END}', 'User-Agent': 'curl/8.5.0'})
    with urllib.request.urlopen(req) as r, open(args.out, 'wb') as f:
        if r.status != 206:  # the CDN sometimes ignores Range on a cold request
            sys.exit(f'server answered {r.status} instead of 206 (range ignored), try again')
        z, crc, n, got = zlib.decompressobj(-15), 0, 0, 0
        while True:
            b = r.read(1 << 20)
            if not b:
                break
            got += len(b)
            d = z.decompress(b)
            crc, n = zlib.crc32(d, crc), n + len(d)
            f.write(d)
            if got % (256 << 20) < len(b):
                print(f'{got / 1e9:.2f} / {(END - START + 1) / 1e9:.2f} GB', file=sys.stderr)
            if args.limit and got >= args.limit * 1e6:
                print(f'stopped after {got} compressed bytes, wrote {n} bytes', file=sys.stderr)
                return
        d = z.flush()
        crc, n = zlib.crc32(d, crc), n + len(d)
        f.write(d)
    if n != SIZE or crc != CRC:
        sys.exit(f'bad result: {n} bytes, crc {crc:08x} (expected {SIZE}, {CRC:08x})')
    print(f'{args.out}: {n} bytes, crc ok')


if __name__ == '__main__':
    main()
