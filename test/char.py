import sys
import os
import telnetlib
import re
import time
import json


ARC_CHAR_SEND="wimpy 0"
ARC_CHAR_MATCH="You will stand your ground no matter what."

LUAI_SEND="sendtochar(self,'BINGOBANGO')"
LUAI_MATCH="BINGOBANGO"

class Character:
    def __init__(self, name, password):
        self.name=name
        self.password=password
        self.lastsend=None
      
    def connect(self, host, port, log=True, logname=None, logwriter=None):
        self.tn=telnetlib.Telnet(host,port)
        self.tn.read_until("With what name do you travel through the realm of Aarchon?")
        self.tn.write(self.name + "\n")
        self.tn.write(self.password +"\n")
        self.tn.write("y\n\n\n")
        # start the log
        if log == True:
            if not logname is None:
                self.logwriter=open(logname, 'w')
            else:
                self.logwriter=open( self.name.capitalize()+"_"+time.strftime("%Y_%m_%d_%H%M%S.txt"), 'w')
        elif not logwriter is None:
            self.logwriter=logwriter
        else:
            self.logwriter=None
                
        self.disable_color()
        self.disable_prompt()
    
    def create_or_connect(self, host="rooflez.com", port=7101, log=True, logname=None, logwriter=None, race="gimp", clss="thief"):
        self.tn=telnetlib.Telnet(host,port)
        self.tn.read_until("With what name do you travel through the realm of Aarchon?")
        self.tn.write(self.name+"\n")
        resp=self.tn.read_until("?")
        if "Did I get that right" in resp:
            self.create(host, port, log, logname, logwriter, race, clss)
        elif "Welcome back" in resp:
            self.connect(host, port, log, logname, logwriter)
        else:
            raise StandardError("No worky: "+resp)
        
    def create(self, host="rooflez.com", port=7101, log=True, logname=None, logwriter=None, race="gimp", clss="thief"):
        self.tn=telnetlib.Telnet(host,port)
        self.tn.read_until("With what name do you travel through the realm of Aarchon?")
        self.tn.write(self.name+"\n")
        self.tn.write("y\n")
        self.tn.write(self.password+"\n")
        self.tn.write(self.password+"\n")
        self.tn.write("N\n")
        self.tn.write("normal\n")
        self.tn.write("m\n")
        self.tn.write("normal\n")
        self.tn.write(clss+"\n")
        self.tn.write(race+"\n")
        self.tn.write("\n\n\n")
        
        # start the log
        if log == True:
            if not logname is None:
                self.logwriter=open(logname, 'w')
            else:
                self.logwriter=open( self.name.capitalize()+"_"+time.strftime("%Y_%m_%d_%H%M%S.txt"), 'w')
        elif not logwriter is None:
            self.logwriter=logwriter
                
        self.disable_color()
        self.disable_prompt()
    
    def remort(self, race="gimp"):
        self.tn.write("\n")
        self.tn.write(race+"\n")
        self.tn.write("default\n")
        self.tn.write("done\n")
        self.send("new i remorted")
        
    def delete(self):
        self.send("delete")
        self.send("delete confirm "+self.password)
    
    def send(self, command, cmd_for_prompt=ARC_CHAR_SEND, prompt=ARC_CHAR_MATCH, timeout=30):
        self.tn.write(command +"\n")
        if cmd_for_prompt:
            self.tn.write(cmd_for_prompt + "\n")
        resp=self.tn.read_until(prompt, timeout)
               
        if not self.logwriter is None:
            tm=time.time()
            dur=0
            if not self.lastsend is None:
               dur=tm-self.lastsend
            self.lastsend=tm
                
            self.logwriter.write("\n"+command +"["+"%.2f"%(dur)+"]\n")
            self.logwriter.write(resp)
            
        return resp 
    
    def disable_prompt(self):
        resp=self.send("prompt")
        if "You will now see prompts." in resp:
            self.disable_prompt()
        elif "You will no longer see prompts." in resp:
            return
        else:
            raise StandardError("Error in DisablePropmt, response did not contain off or on responses.")

    def disable_color(self):
        resp=self.send("color")
        if "Way Cool!" in resp: # we turned it on
            self.disable_color()
        elif "Colour is now OFF" in resp: # we turned it off
            return
        else:
            raise StandardError("Error in DisableColor, response did not contain off or on responses.")
        
    
    def quit(self):
        resp=self.send("quit")
        if not self.logwriter is None:
            self.logwriter.close()
        return resp
    
    def clear_group(self):
        resp=self.send("nofol")
        if "You no longer accept followers." in resp:
            self.send("nofol")
        elif "You now accept followers." in resp:
            self.send("nofol")
            self.send("nofol")
        else:
            raise StandardError("Error in ClearGroup, unexpected nofol response.")
    
    def gag_off(self):
        resp=self.send("gag")
        matches=re.findall("(\w+): (\w+)", resp)
        for match in matches:
            if match[1]=="ON":
                self.send("gag "+match[0])


# Immortals can do anything a character can + more!
class Immortal(Character):
    def __init__(self, name, password):
        Character.__init__(self, name, password)
    
    def open_luai(self):
        self.send("luai", cmd_for_prompt=LUAI_SEND, prompt=LUAI_MATCH)

    def send_luai(self, cmd):
        return self.send(cmd, cmd_for_prompt=LUAI_SEND, 
                prompt=LUAI_MATCH)

    def close_luai(self):
        self.send("@")

    def luai_get_val(self, expr):
        if os.path.exists("../../area/arc_text.json"):
            os.remove("../../area/arc_test.json")
        cmd = "dump_JSON({}, 'arc_test.json')".format(expr)
        self.send_luai(cmd)
        val = json.load(open("../../area/arc_test.json","r"))
        return val

    '''
    def luai_get_val(self, expr, type_=None):
        cmd = "sendtochar(self, '<[|'..tostring({})..'|]>')".format(expr)
        resp = self.send_luai(cmd)
        m = re.search("\<\[\|(.+)\|\]\>", resp)
        val = m.group(1)
        if type_ is not None:
            val = type_(val)
        return val

    def luai_get_table(self, expr):
        env = {'id':0}
        getiter_func = """
                        function getiter(tbl)
                            local k, v
                            local function iter()
                                k,v = next(tbl, k)
                                return k,v
                            end
                            return iter
                        end
                        """
        self.send_luai(getiter_func)

        def get_table(d, tbl_expr):
            env['id'] += 1 
            tbl_name = "TBL{}".format(env['id'])
            iter_name = "ITER{}".format(env['id'])
            k_name = "K{}".format(env['id'])
            v_name = "V{}".format(env['id'])

            self.send_luai("{} = {}".format(tbl_name, tbl_expr))
            self.send_luai("{} = getiter({})".format(iter_name, tbl_name))
            while True:
                self.send_luai("{},{} = {}()".format(k_name, v_name, iter_name))
                
                ktype = self.luai_get_val("type({})".format(k_name)) 
                if ktype == "nil":
                    break
                
                kval = self.luai_get_val(k_name)
            
                vtype = self.luai_get_val("type({})".format(v_name))

                if vtype == "table":
                    d[kval] = {}
                    get_table(d[kval], v_name)
                else:
                    vval = self.luai_get_val(v_name)
                    d[kval] = vval


            env['id'] -= 1 
           
        rtn = {}
        get_table(rtn, expr)
            
        return rtn
    '''
    def clear_mprogs(self, vnum):
        self.medit(vnum)
        for x in range(20): #hopefully no more than 20 mprogs...
            resp=self.send("show")
            if "MOBPrograms for" in resp: # still has some
                self.send("delmp 0")
            else:
                break
        else:
            raise StandardError("Couldn't clear mprogs.")
            
        self.send("done")
            
    def clear_oprogs(self, vnum):
        self.Oedit(vnum)
        for x in range(20): #hopefully no more than 20 oprogs...
            resp=self.send("show")
            if "OBJPrograms for" in resp: #still has some
                self.send("delop 0")
            else:
                break
        else:
            raise StandardError("Couldn't clear oprogs.")
            
        self.send("done")

    def clear_aprogs(self):
        self.Aedit()
        for x in range(20):
            resp=self.send("show")
            if "AREAPrograms for" in resp: #still has some
                self.send("delap 0")
            else:
                break
        else:
            raise StandardError("Couldn't clear aprogs.")
        
        self.send("done")
    
    def asave_changed(self):
        resp=self.send("asave changed")
        if not "Saved zones:" in resp:
            raise StandardError("'asave changed' failed: %s"%(resp))
        
        return resp
    
    def medit(self, mvnum):
        if "That vnum does not exist." in self.send("medit "+str(mvnum)):
            raise StandardError("Can't open medit.")
    
    def oedit(self, ovnum):
        if "That vnum does not exist." in self.send("oedit "+str(ovnum)):
            raise StandardError("Can't open oedit.")
            
    def aedit(self):
        self.send("aedit")
            
    def add_mprog(self, mvnum, mpvnum, trigtype, arg):
        self.medit(mvnum)
        resp=self.send("addmp %s %s '%s'"%(mpvnum, trigtype, arg))
        if "Mprog Added." not in resp:
            raise StandardError("Couldn't add mprog.")
        
        self.send("done")
    
    def add_oprog(self, ovnum, opvnum, trigtype, arg):
        self.Oedit(ovnum)
        resp=self.send("addop %s %s '%s'"%(opvnum, trigtype, arg))
        if "Oprog Added." not in resp:
            raise StandardError("Couldn't add oprog.")
        
        self.send("done")
        
    def add_aprog(self, apvnum, trigtype, arg):
        self.Aedit()
        resp=self.send("addap %s %s '%s'"%(apvnum, trigtype, arg))
        if "Aprog Added." not in resp:
            raise StandardError("Couldn't add aprog.")
    
    def advance(self, name, level):
        if "Raising a player's level!" not in self.send("advance "+name+" "+str(level)):
            raise StandardError("Couldn't advance %s to %s"%(name, level))
    
    def qset(self, chname, qset, val, timer=None):
        if "You have successfully changed" not in self.send("qset %s %s %s %s"%(chname, qset, val, "" if timer is None else timer) ):
            raise StandardError("Couldn't do qset.")
    
    def set_ch_align(self, chname, align):
        return self.send("set char %s align %s"%(chname, align))
    
    def set_act(self, mvnum, flag):
        self.medit(mvnum)
        resp=self.send("show")
        flags=re.search("Act:\s+\[([^\]]+)\]", resp).group(1)
        if flag in flags:
            self.send("done")
            return
        
        resp=self.send("act "+flag)
        if not "Act flag toggled." in resp:
            raise StandardError("Couldn't set act flag: "+flag)
        
        self.send("done")
        
    def remove_act(self, mvnum, flag):
        self.medit(mvnum)
        resp=self.send("show")
        flags=re.search("Act:\s+\[([^\]]+)\]", resp).group(1)
        if flag not in flags:
            self.send("done")
            return
        
        resp=self.send("act "+flag)
        if not "Act flag toggled." in resp:
            raise StandardError("Couldn't set act flag: "+flag)
    
        self.send("done")
        
    def set_hp(self, mvnum, percent):
        self.medit(mvnum)
        resp=self.send("hitpoints "+str(percent))
        
        if "Set hitpoints." not in resp:
            raise StandardError("Couldn't set hitpoints.")
            
        self.send("done")

    def set_obj_owner( self, object, owner=None):
        if owner==None:
            owner=self.name
        
        resp=self.send("set obj "+object+" owner "+owner)
        
        if "Owner set." not in resp:
            raise StandardError("Couldn't set owner.")
        
    def set_obj_clan( self, ovnum, clan):
        self.Oedit(ovnum)
        if clan=="":
            self.send("clan")
        else:
            self.send("clan "+clan)
            resp=self.send("show")
            m=re.search("Clan:\s+\[(\w+)\]", resp)
            if m is None:
                raise StandardError("Couldn't set clan "+clan+" for object "+str(ovnum))
            elif m.group(1) != clan.lower().strip():
                raise StandardError("Couldn't set clan "+clan+" for object "+str(ovnum))
                
        self.send("done")
        
    def goto_room(self, rvnum):
        resp=self.send("goto "+str(rvnum))
        if "[Room "+str(rvnum) not in resp:
            raise StandardError("Couldn't goto room "+str(rvnum))
        
        return resp # just in case they want the response
        
    def load_mob(self, mvnum):
        resp=self.send("load mob "+str(mvnum))
        if "You have created" not in resp:
            raise StandardError("Couldn't load mob "+str(mvnum))
            
        return resp # just in case they want the response
        
    def load_obj(self, ovnum):
        resp=self.send("load obj "+str(ovnum))
        if "You have created" not in resp:
            raise StandardError("Couldn't load obj "+str(ovnum))
        
    def purge_room(self, rvnum=None):
        if rvnum is None:
            resp=self.send("purge")
            if "Ok." not in resp:
                raise StandardError("Couldn't purge room")
            
            return resp # just in case they want the response
        else:
            return self.AtRoom(rvnum, self.PurgeRoom)
        
    def at_room(self, rvnum, fun, *args):
        orig=self.MobStat(self.name)["room"]
        self.GotoRoom(rvnum)
        
        rtn=fun(*args)
        
        self.GotoRoom(orig)
        
        return rtn 
        
    def disable_auth(self):
        for tries in range(3):
            resp=self.send("auth toggle")
            if "Name authorization is now disabled." in resp:
                break
        else:
            raise StandardError("Couldn't disable auth.")

    def void_char(self, name):
        resp=self.send("void "+name)
        if "will void on next tick" not in resp:
            raise StandardError("Couldn't void out "+name)
        
        return resp
            
    def qlist(self, name):
        text = self.send("qlist "+name)
        
        rtn={}
        
        matches=re.findall("(\d+)\s+(\d+)\s+(\d+)", text)
        
        for match in matches:
            rtn[int(match[0])]={'value':int(match[1]), 'timer':int(match[2])}
        
        return rtn
    
    
    def area_stat(self):
        self.send("aedit")
        text = self.send("show")
        self.send("done")
        
        rtn={}
        m=re.search("Name:\s+\[[^\]]+\](.*)", text)
        rtn["name"]=m.group(1).strip()
        
        m=re.search("File:\s+(.*)", text)
        rtn["filename"]=m.group(1).strip()
        
        m=re.search("Players:\s+\[(.*)\]", text)
        rtn["nplayer"]=int(m.group(1))
        
        m=re.search("Min Level:\s+\[(\d+)\]", text)
        rtn["minlevel"]=int(m.group(1))
        
        m=re.search("Max Level:\s+\[(\d+)\]", text)
        rtn["maxlevel"]=int(m.group(1))
        
        return rtn
    
    def obj_stat(self, obj):
        text = self.send("stat obj "+obj)
        if "Nothing like that in hell, earth, or heaven." in text:
            raise StandardError("Couldn't find object: "+obj)
        
        rtn={}
        m=re.search("Name\(s\): (.*)\s+Owner:", text)
        rtn["name"]=m.group(1).strip()
        
        m=re.search("Owner: (.*)", text)
        rtn["owner"]=m.group(1)
        
        m=re.search("Vnum: (\d+)", text)
        rtn["vnum"]=int(m.group(1))
        
        m=re.search("Type: (\w+)", text)
        rtn["otype"]=m.group(1)
        
        m=re.search("Short description: (.*)", text)
        rtn["shortdescr"]=m.group(1)
        
        m=re.search("Long description: (.*)", text)
        rtn["longdescr"]=m.group(1)
        
        m=re.search("Wear bits: (.*)", text)
        rtn["wear"]=m.group(1).split()
        
        m=re.search("Extra bits: (.*)", text)
        rtn["extra"]=m.group(1).split()
        
        m=re.search("Level: (\d+)", text)
        rtn["level"]=int(m.group(1))
        
        m=re.search("Weight: (\d+)/", text)
        rtn["weight"]=int(m.group(1))
        
        m=re.search("Cost: (\d+)", text)
        rtn["cost"]=int(m.group(1))
        
        m=re.search("In room: (\d+)\s+In object: (.*)\s+Carried by: (.*)\s+Wear_loc: (-?\d+)", text)
        rtn["inroom"]=int(m.group(1))
        rtn["inobj"]=m.group(2)
        rtn["carriedby"]=m.group(3)
        rtn["wearloc"]=int(m.group(4))
        
        m=re.search("Values: (\d+) (\d+) (\d+) (\d+) (\d+)", text)
        rtn["v0"]=int(m.group(1))
        rtn["v1"]=int(m.group(2))
        rtn["v2"]=int(m.group(3))
        rtn["v3"]=int(m.group(4))
        rtn["v4"]=int(m.group(5))
                
        m=re.search("Clan: (\w+)", text)
        rtn["clan"]=m.group(1)
        
        m=re.search("ClanRank: (\d+)", text)
        rtn["clanrank"]=int(m.group(1))
        
        m=re.search("Material: (\w+)", text)
        rtn["material"]=m.group(1)
        
        return rtn
    
    def mob_stat(self, mob):
        text = self.send("stat mob "+mob)
        
        rtn={}
        m=re.search("Hp:\s+(-?\d+)/(-?\d+)", text)
        rtn["hp"]=int(m.group(1))
        rtn["maxhp"]=int(m.group(2))
        
        m=re.search("Mana:\s+(-?\d+)/(-?\d+)", text)
        rtn["mana"]=int(m.group(1))
        rtn["maxmana"]=int(m.group(2))
        
        m=re.search("Move:\s+(-?\d+)/(-?\d+)", text)
        rtn["move"]=int(m.group(1))
        rtn["maxmove"]=int(m.group(2))
        
        m=re.search("Name:\s+(\w+)", text)
        rtn["name"]=m.group(1)

        m=re.search("Lvl:\s+(-?\d+)", text)
        rtn["level"]=int(m.group(1))
        
        m=re.search("Gold:\s+(-?\d+)", text)
        rtn["gold"]=int(m.group(1))
        
        m=re.search("Silver:\s+(-?\d+)", text)
        rtn["silver"]=int(m.group(1))
        
        rtn["money"]=rtn["gold"]*100 + rtn["silver"]
        
        m=re.search("Sex:\s+(\w+)", text)
        rtn["sex"]=m.group(1)
        
        m=re.search("Size:\s+(\w+)", text)
        rtn["size"]=m.group(1)
        
        m=re.search("Align:\s+(-?\d+)", text)
        rtn["align"]=int(m.group(1))
        
        m=re.search("Str:\s+-?\d+\((-?\d+)\)", text)
        rtn["str"]=int(m.group(1))
                
        m=re.search("Con:\s+-?\d+\((-?\d+)\)", text)
        rtn["con"]=int(m.group(1))
        
        m=re.search("Vit:\s+-?\d+\((-?\d+)\)", text)
        rtn["vit"]=int(m.group(1))
        
        m=re.search("Agi:\s+-?\d+\((-?\d+)\)", text)
        rtn["agi"]=int(m.group(1))
        
        m=re.search("Dex:\s+-?\d+\((-?\d+)\)", text)
        rtn["dex"]=int(m.group(1))
        
        m=re.search("Int:\s+-?\d+\((-?\d+)\)", text)
        rtn["int"]=int(m.group(1))
        
        m=re.search("Wis:\s+-?\d+\((-?\d+)\)", text)
        rtn["wis"]=int(m.group(1))
        
        m=re.search("Dis:\s+-?\d+\((-?\d+)\)", text)
        rtn["dis"]=int(m.group(1))
        
        m=re.search("Cha:\s+-?\d+\((-?\d+)\)", text)
        rtn["cha"]=int(m.group(1))
        
        m=re.search("Luc:\s+-?\d+\((-?\d+)\)", text)
        rtn["luc"]=int(m.group(1))
        
        m=re.search("Clan:\s+(\w+)", text)
        if not m is None:
            rtn["clan"]=m.group(1)
            
        m=re.search("Rank:\s+(\w+)", text)
        if not m is None:
            rtn["clanrank"]=m.group(1)
        
        m=re.search("Class:\s+(\w+)", text)
        rtn["class"]=m.group(1)
        
        m=re.search("Race:\s+(\w+)", text)
        rtn["race"]=m.group(1)

        m=re.search("Room:\s+(\d+)", text)
        rtn["room"]=int(m.group(1))

        m = re.search("Armor:\s+(-?\d+)", text)
        rtn['ac'] = int(m.group(1))

        m = re.search("Hit: (\d+)\s*Dam: (\d+)\s*Saves: (-?\d+)\s*"
                      "Physical: (-?\d+)\s*Size: (\w+)\s*Position: (\w+)",
                      text)
        if m is not None:
            rtn['hitroll'] = int(m.group(1))
            rtn['damroll'] = int(m.group(2))
            rtn['savesmagic'] = int(m.group(3))
            rtn['savesphys'] = int(m.group(4))
            rtn['size'] = m.group(5)
            rtn['position'] = m.group(6)

        m = re.search("Fighting:\s*(\S+)\s*Wait:\s*(\d+)\s*Daze:\s*(\d+)\s*Stop:\s*(\d+)", text)
        if m is not None:
            rtn['waitcount'] = int(m.group(2))
            rtn['stopcount'] = int(m.group(4))

        m = re.search("God:\s*(\w+)\s+Rank:\s*(\w+)\s*Faith:\s*(\d+)", text)
        if m is not None:
            rtn['godname'] = None if m.group(1) == "None" else m.group(1)
            rtn['religionrank'] = None if m.group(2) == "None" else m.group(2)
            rtn['faith'] = int(m.group(3))

        m = re.search("Damage:\s+(\d+)d(\d+)", text)
        if m is not None:
            rtn['dicenumber'] = int(m.group(1))
            rtn['dicetype'] = int(m.group(2))
        
        m = re.search("Message:\s+(.*)\n", text)
        if m is not None:
            rtn['attacktype']=m.group(1)
        
        m=re.search("Short description:\s+(.*)\n", text)
        if not m is None:
            rtn["shortdescr"]=m.group(1)
        
        m=re.search("Long\s+description:\s+(.*)\n", text)
        if not m is None:
            rtn["longdescr"]=m.group(1)
            
        m=re.search("Mobkills:\s+(\d+)", text)
        if not m is None:
            rtn["mobkills"]=int(m.group(1))
            
        m=re.search("Mobdeaths:\s+(\d+)", text)
        if not m is None:
            rtn["mobdeaths"]=int(m.group(1))
            
        m=re.search("Pkills:\s+(\d+)", text)
        if not m is None:
            rtn["pkills"]=int(m.group(1))
        
        m=re.search("Pkdeaths:\s+(\d+)", text)
        if not m is None:
            rtn["pkdeaths"]=int(m.group(1))

        m=re.search("QPoints:\s+(\d+)", text)
        if not m is None:
            rtn["questpoints"]=int(m.group(1))
            
        m=re.search("Vnum:\s+(\d+)", text)
        if not m is None:
            rtn["vnum"]=int(m.group(1))
            
        m=re.search("Beheads:\s+(\d+)",  text)
        if not m is None:
            rtn["beheads"]=int(m.group(1))
            
        m=re.search("Remorts:\s+(\d+)", text)
        if not m is None:
            rtn["remorts"]=int(m.group(1))

        m=re.search("Bank:\s+(\d+)", text)
        if not m is None:
            rtn["bank"]=int(m.group(1))
            
        m=re.search("Act:(.*)", text)
        if not m is None:
            rtn["act"]=m.group(1).split()
            
        return rtn
