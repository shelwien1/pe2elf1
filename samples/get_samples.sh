#!/bin/sh
#  Download the public E-AC-3 samples into DIR (default ./corpus), checking each against the MD5 in
#  samples.md5 beside this script:
#    - samples.ffmpeg.org/A-codecs/AC3/eac3/: demo-disc and broadcast rips, 2007-2010 (AHT, SPX,
#      dependent substreams for 7.1, 1-block frames at 1536 kbps);
#    - the Dolby Digital Plus Online Delivery Kit v1.5 (current Dolby encoder output): four clear
#      fMP4 audio tracks of its HLS example stream, fetched out of the 3 GB zip by HTTP range
#      requests (its central directory, then each member), and demuxed to elementary streams with
#      ffmpeg (-c:a copy -f eac3).
#  Needs curl, python3 and ffmpeg.
#
#      ./get_samples.sh [DIR]
set -e
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
dir=${1:-corpus}
mkdir -p "$dir"
cd "$dir"

get() {  # url file: with retries, resumed
  for i in 1 2 3 4 5 6; do
    if curl -sS -f -m 900 -C - -o "$2" "$1"; then return 0; fi
    echo "get_samples.sh: retry $i: $2" >&2
    sleep $((i * 3))
  done
  return 1
}

FF=https://samples.ffmpeg.org/A-codecs/AC3/eac3
for f in csi_miami_stereo_128_spx.eac3 csi_miami_5.1_256_spx.eac3 rio_bravo_mono_64_spx.ac3 \
         matrix2_commentary1_stereo_192.eac3 casablanca_aht_mono_64.eac3 buggy_aht_5.1_384.eac3 \
         matrix2_english_5.1_640.eac3 serenity_english_5.1_1536.eac3 7_pt_1.eac3; do
  [ -s "$f" ] || get "$FF/$f" "$f"
done
[ -s 00007.ac3 ] || get "$FF/dependentsubstreams/00007.ac3" 00007.ac3

#  The Dolby kit: the zip's tail (its central directory), then each member by its local header
DK=https://ott.dolby.com/OnDelKits/DDP/Dolby_Digital_Plus_Online_Delivery_Kit_example_stream_HLS_v1.5.zip
need=0
for t in 2ch_128kbps_ddp 6ch_256kbps_ddp 514ch_448kbps_ddp 6ch_640kbps_ddp_joc; do
  [ -s "dolby_Holi_en_$t.eac3" ] || need=1
done
if [ $need = 1 ]; then
  python3 -I - "$DK" <<'PY'
import struct, subprocess, sys, zlib
url = sys.argv[1]
def fetch(a, b):
    return subprocess.run(['curl', '-sS', '-f', '-m', '900', '-r', '%d-%d' % (a, b), url],
                          capture_output=True, check=True).stdout
size = int([l for l in subprocess.run(['curl', '-sS', '-I', '-L', url], capture_output=True, check=True)
            .stdout.decode('latin1').splitlines() if l.lower().startswith('content-length')][-1].split()[1])
tail = fetch(size - 4000000, size - 1)
base = size - len(tail)
j = tail.rfind(b'PK\x06\x06')
nent, cdsize, cdoff = struct.unpack('<QQQ', tail[j + 32:j + 56])
p = cdoff - base
want = {'Holi_en_%s_sub_clean.mp4' % t for t in
        ('2ch_128kbps_ddp', '6ch_256kbps_ddp', '514ch_448kbps_ddp', '6ch_640kbps_ddp_joc')}
for _ in range(nent):
    (sig, vm, vn, fl, meth, t, d, crc, csz, usz, nl, xl, cl, dn, ia, ea, lho) = \
        struct.unpack('<IHHHHHHIIIHHHHHII', tail[p:p + 46])
    name = tail[p + 46:p + 46 + nl].decode('utf8', 'replace')
    extra = tail[p + 46 + nl:p + 46 + nl + xl]
    q = 0
    while q + 4 <= len(extra):
        hid, hsz = struct.unpack('<HH', extra[q:q + 4])
        if hid == 1:
            vals = [struct.unpack('<Q', extra[q + 4 + r:q + 12 + r])[0] for r in range(0, hsz, 8)]
            if usz == 0xFFFFFFFF: usz = vals.pop(0)
            if csz == 0xFFFFFFFF: csz = vals.pop(0)
            if lho == 0xFFFFFFFF: lho = vals.pop(0)
        q += 4 + hsz
    p += 46 + nl + xl + cl
    base_name = name.rsplit('/', 1)[-1]
    if '/Audio_fMP4/' not in name or base_name not in want:
        continue
    raw = fetch(lho, lho + 30 + 1024 + csz)
    nl2, xl2 = struct.unpack('<HH', raw[26:30])
    comp = raw[30 + nl2 + xl2:30 + nl2 + xl2 + csz]
    data = zlib.decompress(comp, -15) if meth == 8 else comp
    if zlib.crc32(data) & 0xFFFFFFFF != crc:
        sys.exit('get_samples.sh: %s: CRC mismatch' % base_name)
    open(base_name, 'wb').write(data)
    print(base_name, len(data))
PY
  for t in 2ch_128kbps_ddp 6ch_256kbps_ddp 514ch_448kbps_ddp 6ch_640kbps_ddp_joc; do
    ffmpeg -hide_banner -loglevel error -y -i "Holi_en_${t}_sub_clean.mp4" -map 0:a:0 -c:a copy -f eac3 \
      "dolby_Holi_en_$t.eac3"
    rm -f "Holi_en_${t}_sub_clean.mp4"
  done
fi

#  the MD5s of what is here, against samples.md5
bad=0
for f in *.eac3 *.ac3; do
  want=$(grep "  $f\$" "$here/samples.md5" | cut -d' ' -f1)
  [ -n "$want" ] || continue
  got=$(md5sum "$f" | cut -d' ' -f1)
  if [ "$got" != "$want" ]; then echo "get_samples.sh: $f: MD5 differs" >&2; bad=1; fi
done
exit $bad
