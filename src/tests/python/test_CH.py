import os
import re
import telnetlib

import pytest

import config



class Player(object):
    def __init__(self, name, password):
        self.name = name
        self.password = password

    def connect(self, host, port):
        self.tn = telnetlib.Telnet(host,port)
        self.tn.read_until(b"With what name do you travel through the realm of Aarchon?")
        self.tn.write(self.name + b"\n")
        self.tn.write(self.password + b"\n")
        self.tn.write(b"y\n\n\n")
        
        self.flog = open(self.name.capitalize(), 'a')

        self.disable_color()
        self.disable_prompt()

    def command(
        self, cmd, cmd_for_prompt=b"wimpy 0", prompt=b"You will stand your ground no matter what.", timeout=30
        ):

        towrite = cmd + b'\n'
        if cmd_for_prompt:
            towrite += cmd_for_prompt + b'\n'
        self.flog.write(towrite.decode("utf-8"))
        self.tn.write(towrite)
        resp = self.tn.read_until(prompt, timeout)
        self.flog.write(resp.decode("utf-8"))
        return resp

    def disable_color(self):
        resp = self.command(b'color')
        if b'Way Cool!' in resp:  # we turned it on
            self.disable_color()
        elif b'Colour is now OFF' in resp:  # we turned it off
            return
        else:
            raise Exception("Unexpected response")

    def disable_prompt(self):
        resp = self.command(b'prompt')
        if b'You will now see prompts.' in resp:
            self.disable_prompt()
        elif b'You will no longer see prompts.' in resp:
            return
        else:
            raise Exception("Unexpected response")

    def luai_script(self, script):
        self.command(b'luai', cmd_for_prompt=None, prompt=b'lua> ')
        return self.command(script, cmd_for_prompt=b'@', prompt=b'Exited lua interpreter.')

    def mob_stat(self, mob):
        text = self.command(b"stat mob " + mob)
        
        rtn = {}
        m = re.search(rb"Hp:\s+(-?\d+)/(-?\d+)", text)
        rtn["hp"] = int(m.group(1))
        rtn["maxhp"] = int(m.group(2))

        m = re.search(rb"Mana:\s+(-?\d+)/(-?\d+)", text)
        rtn["mana"] = int(m.group(1))
        rtn["maxmana"] = int(m.group(2))
        
        m = re.search(rb"Move:\s+(-?\d+)/(-?\d+)", text)
        rtn["move"] = int(m.group(1))
        rtn["maxmove"] = int(m.group(2))

        m = re.search(rb"Name:\s+(\w+)", text)
        rtn["name"] = m.group(1)

        m = re.search(rb"Lvl:\s+(-?\d+)", text)
        rtn["level"] = int(m.group(1))

        m = re.search(rb"Gold:\s+(-?\d+)", text)
        rtn["gold"] = int(m.group(1))
        
        m = re.search(rb"Silver:\s+(-?\d+)", text)
        rtn["silver"] = int(m.group(1))
        
        rtn["money"] = rtn["gold"] * 100 + rtn["silver"]

        m = re.search(rb"Sex:\s+(\w+)", text)
        rtn["sex"] = m.group(1)
        
        m = re.search(rb"Size:\s+(\w+)", text)
        rtn["size"] = m.group(1)
        
        m = re.search(rb"Align:\s+(-?\d+)", text)
        rtn["align"] = int(m.group(1))

        m = re.search(rb"Str:\s+-?\d+\((-?\d+)\)", text)
        rtn["str"] = int(m.group(1))
                
        m = re.search(rb"Con:\s+-?\d+\((-?\d+)\)", text)
        rtn["con"] = int(m.group(1))
        
        m = re.search(rb"Vit:\s+-?\d+\((-?\d+)\)", text)
        rtn["vit"] = int(m.group(1))
        
        m = re.search(rb"Agi:\s+-?\d+\((-?\d+)\)", text)
        rtn["agi"] = int(m.group(1))
        
        m = re.search(rb"Dex:\s+-?\d+\((-?\d+)\)", text)
        rtn["dex"] = int(m.group(1))
        
        m = re.search(rb"Int:\s+-?\d+\((-?\d+)\)", text)
        rtn["int"] = int(m.group(1))
        
        m = re.search(rb"Wis:\s+-?\d+\((-?\d+)\)", text)
        rtn["wis"] = int(m.group(1))
        
        m = re.search(rb"Dis:\s+-?\d+\((-?\d+)\)", text)
        rtn["dis"] = int(m.group(1))
        
        m = re.search(rb"Cha:\s+-?\d+\((-?\d+)\)", text)
        rtn["cha"] = int(m.group(1))
        
        m = re.search(rb"Luc:\s+-?\d+\((-?\d+)\)", text)
        rtn["luc"] = int(m.group(1))
        
        m = re.search(rb"Clan:\s+(\w+)", text)
        if not m is None:
            rtn["clan"] = m.group(1)
            
        m = re.search(rb"Rank:\s+(\w+)", text)
        if not m is None:
            rtn["clanrank"] = m.group(1)
        
        m = re.search(rb"Class:\s+(\w+)", text)
        rtn["class"] = m.group(1)
        
        m = re.search(rb"Race:\s+(\w+)", text)
        rtn["race"] = m.group(1)

        m = re.search(rb"Room:\s+(\d+)", text)
        rtn["room"] = int(m.group(1))
        
        m = re.search(rb"Short description:\s+(.*)\n", text)
        if not m is None:
            rtn["shortdescr"] = m.group(1)
        
        m = re.search(rb"Long\s+description:\s+(.*)\n", text)
        if not m is None:
            rtn["longdescr"] = m.group(1)
            
        m = re.search(rb"Position:\s+(.*)\n", text)
        rtn["position"] = m.group(1)
        
        m = re.search(rb"Mobkills:\s+(\d+)", text)
        if not m is None:
            rtn["mobkills"] = int(m.group(1))
            
        m = re.search(rb"Mobdeaths:\s+(\d+)", text)
        if not m is None:
            rtn["mobdeaths"] = int(m.group(1))
            
        m = re.search(rb"Pkills:\s+(\d+)", text)
        if not m is None:
            rtn["pkills"] = int(m.group(1))
        
        m = re.search(rb"Pkdeaths:\s+(\d+)", text)
        if not m is None:
            rtn["pkdeaths"] = int(m.group(1))

        m = re.search(rb"QPoints:\s+(\d+)", text)
        if not m is None:
            rtn["questpoints"] = int(m.group(1))
            
        m = re.search(rb"Vnum:\s+(\d+)", text)
        if not m is None:
            rtn["vnum"] = int(m.group(1))
            
        m = re.search(rb"Beheads:\s+(\d+)",  text)
        if not m is None:
            rtn["beheads"] = int(m.group(1))
            
        m = re.search(rb"Remorts:\s+(\d+)", text)
        if not m is None:
            rtn["remorts"] = int(m.group(1))

        m = re.search(rb"Bank:\s+(\d+)", text)
        if not m is None:
            rtn["bank"] = int(m.group(1))

        return rtn


@pytest.mark.parametrize(
    'prop_name, prop_type', [
        ('hp', int),
        ('maxhp', int),
        ('mana', int),
        ('maxmana', int),
        ('move', int),
        ('maxmove', int),
        ('gold', int),
        ('silver', int),
        ('money', int),
        ('sex', None),
        ('size', None),
        ('align', int),
        ('str', int),
        ('con', int),
        ('vit', int),
        ('agi', int),
        ('vit', int),
        ('dex', int),
        ('int', int),
        ('wis', int),
        ('dis', int),
        ('cha', int),
        ('luc', int),
        ('class', None),
        ('race', None),
        ('name', None),
        ('level', int),
        ('remorts', int),
        ('beheads', int),
        ('pkills', int),
        ('pkdeaths', int),
        ('questpoints', int),
        ('bank', int),
        ('mobkills', int),
        ('mobdeaths', int),
    ]
)
def test_CH_get(prop_name, prop_type):
    pl = Player(config.IMM_CHAR.encode(), config.IMM_CHAR_PW.encode())
    pl.connect(config.TEST_HOST.encode(), config.TEST_PORT.encode())

    mstat = pl.mob_stat(pl.name)
    resp = pl.luai_script(b'say(tostring(mob.' + prop_name.encode() + b'))')

    lua_val = re.search(rb"You say '(-?\w+)'", resp).group(1)
    if prop_type:
        lua_val = prop_type(lua_val)
    assert mstat[prop_name] == lua_val


@pytest.mark.parametrize(
    'prop_name', [
        ('hp'),
        ('mana'),
        ('move'),
    ]
)
def test_CH_set_hmv(prop_name):
    pl = Player(config.IMM_CHAR.encode(), config.IMM_CHAR_PW.encode())
    pl.connect(config.TEST_HOST.encode(), config.TEST_PORT.encode())

    pl.luai_script(b'mob.' + prop_name.encode() + b' = 1')
    mstat = pl.mob_stat(pl.name)
    assert mstat[prop_name] == 1

    pl.luai_script(b'mob.' + prop_name.encode() + b' = 9999')
    mstat = pl.mob_stat(pl.name)
    assert mstat[prop_name] == 9999