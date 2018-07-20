#!/usr/bin/env python3
import os
from pathlib import Path
from unittest.mock import patch
import re


@patch('setuptools.setup')
def get_package_data(mock_setup):
    import static.Netron.setup as netron_setup
    package_data = mock_setup.call_args[1]['package_data']['netron']
    return (package_data, netron_setup.node_dependencies[0][1])


def get_netron_static_scripts(src_path):
    with open(src_path / 'view-browser.html') as f:
        scripts = []
        regex = re.compile("<script type='text/javascript' src='(.*)'></script>")
        for line in f.readlines():
            match = re.match(regex, line)
            if match:
                scripts.append(match.group(1))
        return scripts


def rebuild_needed(sources, destination):
    try:
        destination_mtime = os.path.getmtime(destination)
    except FileNotFoundError:
        return True
    return any(os.path.getmtime(f) >= destination_mtime for f in sources)


def bundle_scripts(files):
    bundle = []
    for script in files:
        with script.open('rb') as f:
            bundle.append(f.read())
    return b'\n'.join(bundle)


def main():
    netron = Path('static/Netron')
    src = netron / 'src'
    package_data, node_dependencies = get_package_data()
    print('Netron package files:\n{}'.format(' '.join(package_data)))
    print('Netron Node dependencies:\n{}'.format(' '.join(node_dependencies)))

    static_scripts = get_netron_static_scripts(src)
    print('These scripts will be bundled:\n{}'.format(' '.join(static_scripts)))
    # Update script paths to point to paths before installation
    for i, script in enumerate(static_scripts):
        if script in package_data:
            static_scripts[i] = src / script
        else:
            for filename in node_dependencies:
                path = Path(filename)
                if script == path.name:
                    static_scripts[i] = netron / path

    package_data = set(package_data) - set(static_scripts)

    ignored_extensions = ['.css', '.html', '.ico']
    package_data = [src / filename for filename in package_data if not Path(filename).suffix in ignored_extensions]

    public = Path('public')
    bundle_destination = public / 'netron_bundle.js'
    if rebuild_needed(static_scripts, bundle_destination):
        try:
            with open(bundle_destination, 'wb') as f:
                f.write(bundle_scripts(static_scripts))
        except:
            # Remove the output file if bundling fails
            try:
                bundle_destination.unlink()
            except:
                pass
            raise
    else:
        print('Bundle is already up to date')

    for package_file in package_data:
        try:
            os.link(package_file, public / package_file.name)
        except FileExistsError:
            pass


if __name__ == '__main__':
    main()
