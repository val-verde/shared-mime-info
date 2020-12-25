#!/usr/bin/env python3

"""
Compare list of IANA media type registrations against ours.
Copyright (C) 2020 Ville Skyttä <ville.skytta@iki.fi>, GPL v2 or later.

Environment:
   IANA_URL: URL to IANA media-types.xml
   FDO_URL : URL to our freedesktop.xml(.in)
"""

import os
from pathlib import Path
import re
from urllib.request import urlopen
import xml.etree.ElementTree as ET


IANA_URL = os.environ.get(
    "IANA_URL", "https://www.iana.org/assignments/media-types/media-types.xml"
)
IANA_XMLNS = "{http://www.iana.org/assignments}"

FDO_URL = os.environ.get(
    "FDO_URL",
    (
        Path(__file__).resolve().parent.parent / "data" / "freedesktop.org.xml.in"
    ).as_uri(),
)
FDO_XMLNS = "{http://www.freedesktop.org/standards/shared-mime-info}"

MIME_TYPE_RE = re.compile(
    r"^[a-z0-9][\w!#$&^.+-]{0,126}/[a-z0-9][\w!#$&^.+-]{0,126}$", re.IGNORECASE
)


with urlopen(IANA_URL) as f:
    tree = ET.parse(f)

iana_types = set()
iana_obsolete = set()
for elem in tree.findall(f"{IANA_XMLNS}registry"):
    main_type = elem.attrib["id"]
    for elem in elem.findall(f"{IANA_XMLNS}record/{IANA_XMLNS}name"):
        whole_type = f"{main_type}/{elem.text}"
        type_words = whole_type.split()
        if any("OBSOLETE" in x or "DEPRECATED" in x for x in type_words):
            mime_type = type_words[0]
            iana_obsolete.add(mime_type)
        else:
            mime_type = whole_type
            iana_types.add(mime_type)
        assert MIME_TYPE_RE.match(mime_type), f"invalid IANA type '{mime_type}'"
assert iana_types

with urlopen(FDO_URL) as f:
    tree = ET.parse(f)

fdo_types = set()
fdo_aliases = set()
for elem in tree.findall(f"{FDO_XMLNS}mime-type"):
    fdo_types.add(elem.attrib["type"])
    for elem in elem.findall(f"{FDO_XMLNS}alias"):
        fdo_aliases.add(elem.attrib["type"])
assert fdo_types
for mime_type in fdo_types:
    assert MIME_TYPE_RE.match(mime_type), f"invalid FDO type '{mime_type}'"
assert fdo_aliases
for mime_type in fdo_aliases:
    assert MIME_TYPE_RE.match(mime_type), f"invalid FDO alias '{mime_type}'"

fdo_types_lower = {x.lower(): x for x in fdo_types}
fdo_aliases_lower = {x.lower(): x for x in fdo_aliases}

for mime_type in sorted(iana_types):
    if mime_type in fdo_types:
        print(f"+ {mime_type} present with us")
    elif mime_type in fdo_aliases:
        print(f"~ {mime_type} present as alias with us")
    elif mime_type.lower() in fdo_types_lower:
        print(
            f"~ {mime_type} present with us, case differs"
            f": {fdo_types_lower[mime_type.lower()]}"
        )
    elif mime_type.lower() in fdo_aliases_lower:
        print(
            f"~ {mime_type} present as alias with us, case differs"
            f": {fdo_aliases_lower[mime_type.lower()]}"
        )
    else:
        print(f"- {mime_type} missing from us")
