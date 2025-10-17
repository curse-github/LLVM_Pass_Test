sudo apt-get install python3 python3-pip pipx
pipx install lit
if ! grep -q "$HOME/.local/bin" "$HOME/.profile"; then
    echo "" >> $HOME/.profile
    echo "# include local bin in PATH, used by pipx" >> $HOME/.profile
    echo "if [ -d \"$HOME/.local/bin\" ] ; then" >> $HOME/.profile
    echo "    PATH=\"$HOME/.local/bin:\$PATH\"" >> $HOME/.profile
    echo 'fi' >> $HOME/.profile
fi
if ! grep -q "$HOME/.local/bin" "$HOME/.bashrc"; then
    echo "" >> $HOME/.bashrc
    echo "# include local bin in PATH, used by pipx" >> $HOME/.bashrc
    echo "if [ -d \"$HOME/.local/bin\" ] ; then" >> $HOME/.bashrc
    echo "    PATH=\"$HOME/.local/bin:\$PATH\"" >> $HOME/.bashrc
    echo 'fi' >> $HOME/.bashrc
fi