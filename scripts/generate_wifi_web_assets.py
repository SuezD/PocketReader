Import("env")

from pathlib import Path


project_dir = Path(env.subst("$PROJECT_DIR"))
source_dir = project_dir / "web" / "wifi"
output_path = project_dir / "include" / "generated" / "WifiWebAssets.h"

assets = (
    ("SETUP_HTML", "setup.html"),
    ("CONNECTING_HTML", "connecting.html"),
    ("SETUP_CSS", "setup.css"),
    ("SETUP_JS", "setup.js"),
    ("CONNECTING_JS", "connecting.js"),
)

lines = ["#pragma once", "", "namespace WifiWebAssets", "{"]

for constant_name, file_name in assets:
    contents = (source_dir / file_name).read_text(encoding="utf-8")

    if ')PRWEB"' in contents:
        raise RuntimeError(f"Reserved raw-string delimiter in {file_name}")

    lines.extend(
        (
            f"    constexpr char {constant_name}[] = R\"PRWEB({contents})PRWEB\";",
            "",
        )
    )

lines.extend(("}", ""))
generated = "\n".join(lines)
output_path.parent.mkdir(parents=True, exist_ok=True)

if not output_path.exists() or output_path.read_text(encoding="utf-8") != generated:
    output_path.write_text(generated, encoding="utf-8", newline="\n")

