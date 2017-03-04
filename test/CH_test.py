import re
import unittest
from test_setup import imm
from char import Character
import test_config

def kill_mobs():
    imm.open_luai()
    imm.send_luai("for _,v in pairs(getmobworld({})) do " 
                    "v:destroy() " 
                    "end".format(test_config.MOB_GET_VNUM))
    imm.close_luai()

def kill_objs():
    imm.open_luai()
    imm.send_luai("for _,v in pairs(getobjworld({})) do " 
                    "v:destroy() " 
                    "end".format(test_config.OBJ_VNUM))
    imm.close_luai()


def get_luai_val(expr):
    imm.open_luai()
    try:
        val = imm.luai_get_val(expr)
    finally:
        imm.close_luai()

    return val


class Get(unittest.TestCase):
    def check_get(self, prop_name):
        val = get_luai_val("self['{}']".format(prop_name))
        stat = imm.mob_stat(imm.name)
        self.assertEqual(val, stat[prop_name])

    def check_mob_get(self, prop_name):
        val = get_luai_val("getmobworld({})[1]['{}']".format(
            test_config.MOB_GET_VNUM, prop_name))
        stat = imm.mob_stat(test_config.MOB_GET_NAME)
        self.assertEqual(val, stat[prop_name])

    def test_CH_get_name(self):
        self.check_get("name")

    def test_CH_get_level(self):
        self.check_get("level")

    def test_CH_get_hp(self):
        self.check_get("hp")

    def test_CH_get_maxhp(self):
        self.check_get("maxhp")

    def test_CH_get_mana(self):
        self.check_get("mana")

    def test_CH_get_maxmana(self):
        self.check_get("maxmana")

    def test_CH_get_move(self):
        self.check_get("move")

    def test_CH_get_maxmove(self):
        self.check_get("maxmove")

    def test_CH_get_gold(self):
        self.check_get("gold")

    def test_CH_get_silver(self):
        self.check_get("silver")

    def test_CH_get_money(self):
        self.check_get("money")

    def test_CH_get_sex(self):
        self.check_get("sex")

    def test_CH_get_size(self):
        self.check_get("size")

    def test_CH_get_position(self):
        imm.open_luai()
        val = imm.luai_get_val("self.position")
        imm.close_luai()
        stat = imm.mob_stat(imm.name)
        self.assertTrue(val in stat['position'])

    def test_CH_get_align(self):
        self.check_get("align")

    def test_CH_get_str(self):
        self.check_get("str")

    def test_CH_get_con(self):
        self.check_get("con")

    def test_CH_get_vit(self):
        self.check_get("vit")

    def test_CH_get_agi(self):
        self.check_get("agi")

    def test_CH_get_dex(self):
        self.check_get("dex")

    def test_CH_get_int(self):
        self.check_get("int")

    def test_CH_get_wis(self):
        self.check_get("wis")

    def test_CH_get_dis(self):
        self.check_get("dis")

    def test_CH_get_cha(self):
        self.check_get("cha")

    def test_CH_get_ac(self):
        self.check_get("ac")

    def test_CH_get_hitroll(self):
        self.check_get("hitroll")

    def test_CH_get_damroll(self):
        self.check_get("damroll")

    def test_CH_get_attacktype(self):
        self.check_mob_get("attacktype")

    def test_CH_get_savesphys(self):
        self.check_get("savesphys")

    def test_CH_get_savesmagic(self):
        self.check_get("savesmagic")

    def test_CH_get_luc(self):
        self.check_get("luc")

    def test_CH_get_clan(self):
        self.check_get("clan")

    def test_CH_get_class(self):
        self.check_get("class")

    def test_CH_get_race(self):
        self.check_get("race")

    def test_CH_get_ispc(self):
        val = get_luai_val("self.ispc")
        self.assertTrue(val)
    
        val = get_luai_val("getmobworld({})[1].ispc".format(test_config.MOB_GET_VNUM))
        self.assertFalse(val)

    def test_CH_get_isnpc(self):
        val = get_luai_val("self.isnpc")
        self.assertFalse(val)
    
        val = get_luai_val("getmobworld({})[1].isnpc".format(test_config.MOB_GET_VNUM))
        self.assertTrue(val)

    def test_CH_get_isgood(self):
        imm.set_ch_align(imm.name, 1000)
        imm.open_luai()
        val = imm.luai_get_val("self.isgood")
        imm.close_luai()
        self.assertEqual(val, True)

        imm.set_ch_align(imm.name, -1000)
        imm.open_luai()
        val = imm.luai_get_val("self.isgood")
        imm.close_luai()
        self.assertEqual(val, False)

    def test_CH_get_isevil(self):
        imm.set_ch_align(imm.name, -1000)
        imm.open_luai()
        val = imm.luai_get_val("self.isevil")
        imm.close_luai()
        self.assertEqual(val, True)

        imm.set_ch_align(imm.name, 1000)
        imm.open_luai()
        val = imm.luai_get_val("self.isevil")
        imm.close_luai()
        self.assertEqual(val, False)

    def test_CH_get_isneutral(self):
        imm.set_ch_align(imm.name, -1000)
        imm.open_luai()
        val = imm.luai_get_val("self.isneutral")
        imm.close_luai()
        self.assertEqual(val, False)

        imm.set_ch_align(imm.name, 1000)
        imm.open_luai()
        val = imm.luai_get_val("self.isneutral")
        imm.close_luai()
        self.assertEqual(val, False)
        
        imm.set_ch_align(imm.name, 0)
        imm.open_luai()
        val = imm.luai_get_val("self.isneutral")
        imm.close_luai()
        self.assertEqual(val, True)

    def test_CH_get_heshe(self):
        DICT={ "male":"he", "female":"she" }
        stat = imm.mob_stat(imm.name)
        val = get_luai_val("self.heshe")
        self.assertEqual(DICT[stat['sex']], val)

    def test_CH_get_himher(self):
        DICT={ "male":"him", "female":"her" }
        stat = imm.mob_stat(imm.name)
        val = get_luai_val("self.himher")
        self.assertEqual(DICT[stat['sex']], val)

    def test_CH_get_hisher(self):
        DICT={ "male":"his", "female":"her" }
        stat = imm.mob_stat(imm.name)
        val = get_luai_val("self.hisher")
        self.assertEqual(DICT[stat['sex']], val)

    def test_CH_get_inventory(self):
        imm.load_obj(10200)
        val = get_luai_val("#self.inventory")
        self.assertTrue(val > 0)

    def test_CH_get_room(self):
        val = get_luai_val("self.room.vnum")
        stat = imm.mob_stat(imm.name)
        self.assertEqual(val, stat['room'])

    def test_CH_get_stance(self):
        imm.send("stance bunny")
        val = get_luai_val("self.stance")
        self.assertEqual(val, "bunny")

    def test_CH_get_waitcount(self):
        self.check_get("waitcount")

    def test_CH_get_stopcount(self):
        self.check_get("stopcount")

    def test_CH_get_scroll(self):
        imm.send("scroll 27")
        self.assertEqual(get_luai_val("self.scroll"), 25)
        imm.send("scroll 30")
        self.assertEqual(get_luai_val("self.scroll"), 28)

    def test_CH_get_godname(self):
        self.check_get("godname")

    def test_CH_get_faith(self):
        self.check_get("faith")

    def test_CH_get_religionrank(self):
        self.check_get("religionrank")

    def test_CH_get_remorts(self):
        self.check_get("remorts")

    def test_CH_get_explored(self):
        val = get_luai_val("self.explored")
        resp = imm.send("explo")
        m = re.search("You have explored (\d+) rooms", resp)
        
        self.assertEqual(val, int(m.group(1)))

    def test_CH_get_beheads(self):
        self.check_get("beheads")

    def test_CH_get_pkills(self):
        self.check_get("pkills")

    def test_CH_get_pkdeaths(self):
        self.check_get("pkdeaths")

    def test_CH_get_questpoints(self):
        self.check_get("questpoints")

    def test_CH_get_bank(self):
        self.check_get("bank")

    def test_CH_get_mobkills(self):
        self.check_get("mobkills")

    def test_CH_get_mobdeaths(self):
        self.check_get("mobdeaths")

    def test_CH_get_descriptor(self):
        val = get_luai_val("self.descriptor.character == self")
        self.assertTrue(val)

    def test_CH_get_vnum(self):
        self.check_mob_get("vnum")

    def test_CH_get_shortdescr(self):
        self.check_mob_get("shortdescr")

    def test_CH_get_longdescr(self):
        self.check_mob_get("longdescr")

    def test_CH_get_dicenumber(self):
        self.check_mob_get("dicenumber")

    def test_CH_get_dicetype(self):
        self.check_mob_get("dicetype")

    def test_CH_get_clanrank(self):
        lua_crank = get_luai_val("self.clanrank")

        stat = imm.mob_stat(imm.name)
        clan = stat['clan']
        rank = stat['clanrank'] 

        resp = imm.send("clanreport %s rank"%(clan)+"\n\n\n\n\n" )
        m = re.search("Detail for rank (\d+) - %s"%(rank.capitalize()), resp)
        stat_crank = int(m.group(1))
        
        self.assertEqual(stat_crank, lua_crank)

    def test_CH_get_isimmort(self):
        val = get_luai_val("self.isimmort")
        self.assertTrue(val)

        char2 = Character("isimmtest", "blah1")
        char2.create_or_connect()

        val = get_luai_val("getpc('{}').isimmort".format(char2.name))
        self.assertFalse(val)
    
        val = get_luai_val("getmobworld({})[1].isimmort".format(
            test_config.MOB_GET_VNUM))
        self.assertFalse(val)

    def test_CH_get_isfollow(self):
        imm.send("fol self")

        val = get_luai_val("self.isfollow")
        self.assertFalse(val)

        imm.load_mob(test_config.MOB_GET_VNUM)
        imm.send("follow " + test_config.MOB_GET_NAME)

        val = get_luai_val("self.isfollow")
        self.assertTrue(val)

    def test_CH_get_isactive(self):
        imm.send("sleep")

        val = get_luai_val("self.isactive")
        self.assertFalse(val)

        imm.send("stand")
        val = get_luai_val("self.isactive")
        self.assertTrue(val)

    def test_CH_get_groupsize(self):
        char2 = Character("grsizetest", "blah1")
        char2.create_or_connect()

        imm.clear_group()
        imm.send("fol self")
        val = get_luai_val("self.groupsize")
        self.assertEqual(val, 0)

        imm.send("trans " + char2.name)
        imm.send("force {} follow {}".format(char2.name, imm.name))
        imm.send("group {}".format(char2.name))

        val = get_luai_val("self.groupsize")
        self.assertEqual(val, 1)

    def test_CH_get_proto(self):
        val = get_luai_val("getmobworld({})[1].proto.vnum == {}".format(
            test_config.MOB_GET_VNUM, test_config.MOB_GET_VNUM))

        self.assertTrue(val)


class Set(unittest.TestCase):

    def check_self_set(self):
        pass

    def check_mob_set(self, prop, set_val):
        script = """
            (function()
                local tgt = self.room:mload({})
                tgt['{}'] = {} 
                local rtn = (tgt['{}'] == {})
                tgt:destroy()
                return rtn
            end)()
        """.format(
                test_config.MOB_GET_VNUM, 
                prop, set_val,
                prop, set_val)

        val = get_luai_val(script)
        self.assertTrue(val)

    def test_CH_set_name(self):
        self.check_mob_set("name", "'blah1'")
        
    def test_CH_set_shortdescr(self):
        self.check_mob_set("shortdescr", "'blah1'")

    def test_CH_set_longdescr(self):
        self.check_mob_set("longdescr", "'blah1'")
        
    def test_CH_set_level(self):
        self.check_mob_set("level", 5)
        self.check_mob_set("level", 6)

    def test_CH_set_hp(self):
        self.check_mob_set("hp", 123)
        self.check_mob_set("hp", 321)

    def test_CH_set_maxhp(self):
        self.check_mob_set("maxhp", 123)
        self.check_mob_set("maxhp", 321)

    def test_CH_set_mana(self):
        self.check_mob_set("mana", 123)
        self.check_mob_set("mana", 321)

    def test_CH_set_maxmana(self):
        self.check_mob_set("maxmana", 123)
        self.check_mob_set("maxmana", 321)

    def test_CH_set_move(self):
        self.check_mob_set("move", 123)
        self.check_mob_set("move", 321)
    
    def test_CH_set_maxmove(self):
        self.check_mob_set("maxmove", 123)
        self.check_mob_set("maxmove", 321)

    def test_CH_set_gold(self):
        self.check_mob_set("gold", 123)
        self.check_mob_set("gold", 321)
    
    def test_CH_set_silver(self):
        self.check_mob_set("silver", 123)
        self.check_mob_set("silver", 321)

    def test_CH_set_size(self):
        self.check_mob_set("size", "'small'")
        self.check_mob_set("size", "'large'")

    def test_CH_set_sex(self):
        self.check_mob_set("sex", "'male'")
        self.check_mob_set("sex", "'female'")

    def test_CH_set_align(self):
        self.check_mob_set("align", 123)
        self.check_mob_set("align", 321)

    def test_CH_set_str(self):
        self.check_mob_set("str", 12)
        self.check_mob_set("str", 120)

    def test_CH_set_con(self):
        self.check_mob_set("con", 12)
        self.check_mob_set("con", 120)
        
    def test_CH_set_vit(self):
        self.check_mob_set("vit", 12)
        self.check_mob_set("vit", 120)
        
    def test_CH_set_agi(self):
        self.check_mob_set("agi", 12)
        self.check_mob_set("agi", 120)
        
    def test_CH_set_dex(self):
        self.check_mob_set("dex", 12)
        self.check_mob_set("dex", 120)
        
    def test_CH_set_int(self):
        self.check_mob_set("int", 12)
        self.check_mob_set("int", 120)
        
    def test_CH_set_wis(self):
        self.check_mob_set("wis", 12)
        self.check_mob_set("wis", 120)
        
    def test_CH_set_dis(self):
        self.check_mob_set("dis", 12)
        self.check_mob_set("dis", 120)
        
    def test_CH_set_cha(self):
        self.check_mob_set("cha", 12)
        self.check_mob_set("cha", 120)
        
    def test_CH_set_luc(self):
        self.check_mob_set("luc", 12)
        self.check_mob_set("luc", 120)

    def test_CH_set_stopcount(self):
        self.check_mob_set("stopcount", 5)
        self.check_mob_set("stopcount", 1)
        
    def test_CH_set_waitcount(self):
        self.check_mob_set("waitcount", 5)
        self.check_mob_set("waitcount", 1)
        
    def test_CH_set_race(self):
        race = get_luai_val("self.race")
        self.check_mob_set("race", "'gimp'")
        self.check_mob_set("race", "'elf'")
        self.check_mob_set("race", "'{}'".format(race))

class Method(unittest.TestCase):
    def test_CH_method_mobhere(self):
        val = get_luai_val("self:mobhere(123)")
        self.assertFalse(val)

        imm.load_mob(test_config.MOB_GET_VNUM)

        val = get_luai_val("self:mobhere({})".format(test_config.MOB_GET_VNUM))
        self.assertTrue(val)

    def test_CH_method_objhere(self):
        kill_objs()

        val = get_luai_val("self:objhere({})".format(test_config.OBJ_VNUM))
        self.assertFalse(val)

        val = get_luai_val("self:objhere('{}')".format(test_config.OBJ_NAME))
        self.assertFalse(val)

        imm.load_obj(test_config.OBJ_VNUM)
        imm.send("drop " + test_config.OBJ_NAME)

        val = get_luai_val("self:objhere({})".format(test_config.OBJ_VNUM))
        self.assertTrue(val)

        val = get_luai_val("self:objhere('{}')".format(test_config.OBJ_NAME))
        self.assertTrue(val)

    def test_CH_method_mobexists(self):
        kill_mobs()

        val = get_luai_val("self:mobexists('{}')".format(test_config.MOB_GET_NAME))
        self.assertFalse(val)

        imm.load_mob(test_config.MOB_GET_VNUM)
        val = get_luai_val("self:mobexists('{}')".format(test_config.MOB_GET_NAME))
        self.assertTrue(val)

    def test_CH_method_objexists(self):
        kill_objs()

        val = get_luai_val("self:objexists('{}')".format(test_config.OBJ_NAME))
        self.assertFalse(val)

        imm.load_obj(test_config.OBJ_VNUM)

        val = get_luai_val("self:objexists('{}')".format(test_config.OBJ_NAME))
        self.assertTrue(val)

    def test_CH_method_affected(self):
        kill_mobs()

        imm.load_mob(test_config.MOB_GET_VNUM)

        val = get_luai_val("getmobworld({})[1]:affected('haste')".format(
            test_config.MOB_GET_VNUM))
        self.assertFalse(val)
        
        for i in range(5):
            imm.send("cast haste " + test_config.MOB_GET_NAME)
        
        val = get_luai_val("getmobworld({})[1]:affected('haste')".format(
            test_config.MOB_GET_VNUM))
        self.assertTrue(val)

    def test_CH_method_offensive(self):
        val = get_luai_val("self:offensive()")
        self.assertEqual(len(val), 0)
        
        val = get_luai_val("getmobworld({})[1]:offensive('bash')".format(
            test_config.MOB_GET_VNUM))
        self.assertTrue(val)

    def test_CH_method_immune(self):
        val = get_luai_val("self:immune()")
        self.assertEqual(len(val), 0)

        val = get_luai_val("self:immune('fire')")
        self.assertFalse(val)

        val = get_luai_val("getmobworld({})[1]:immune('fire')".format(
            test_config.MOB_GET_VNUM))
        self.assertFalse(val)

        val = get_luai_val("getmobworld({})[1]:immune('summon')".format(
            test_config.MOB_GET_VNUM))
        self.assertTrue(val)

    def test_CH_method_resist(self):
        val = get_luai_val("self:resist()")
        self.assertTrue(type(val) is list)

        val = get_luai_val("getmobworld({})[1]:resist('fire')".format(
            test_config.MOB_GET_VNUM))
        self.assertFalse(val)

    def test_CH_method_vuln(self):
        val = get_luai_val("self:vuln()")
        self.assertTrue(type(val) is list)

        val = get_luai_val("getmobworld({})[1]:vuln('fire')".format(
            test_config.MOB_GET_VNUM))
        self.assertFalse(val)

    def test_CH_method_destroy(self):
        imm.load_mob(test_config.MOB_GET_VNUM)
        val = get_luai_val("#getmobworld({})".format(
            test_config.MOB_GET_VNUM))
        self.assertTrue( val > 0 )

        kill_mobs()

        val = get_luai_val("#getmobworld({})".format(
            test_config.MOB_GET_VNUM))
        self.assertEqual(val, 0)
        imm.load_mob(test_config.MOB_GET_VNUM)

    def test_CH_method_oload(self):
        kill_objs()

        val = get_luai_val("#getobjworld({})".format(
            test_config.OBJ_VNUM))
        self.assertEqual(val, 0)

        imm.load_obj(test_config.OBJ_VNUM)
        val = get_luai_val("#getobjworld({})".format(
            test_config.OBJ_VNUM))
        self.assertEqual(val, 1)

        val = get_luai_val("getobjworld({})[1].carriedby == self".format(
            test_config.OBJ_VNUM))
        self.assertTrue(val)

    def test_CH_method_say(self):
        imm.open_luai()
        resp = imm.send_luai("say('pumpkins')")
        imm.close_luai()

        self.assertTrue("You say 'pumpkins'" in resp)

    def test_CH_method_emote(self):
        imm.open_luai()
        resp = imm.send_luai("emote('in pyjamas')")
        imm.close_luai()

        self.assertTrue("in pyjamas" in resp)

    def test_CH_method_mdo(self):
        imm.open_luai()
        resp = imm.send_luai("mdo('dance')")
        imm.close_luai()

        self.assertTrue("Feels silly, doesn't it?" in resp)

    def test_CH_method_asound(self):
        kill_mobs()
        imm.load_mob(test_config.MOB_GET_VNUM)
        imm.open_luai()
        resp = imm.send_luai("getmobworld({})[1]:asound('ASOUND TEST')")
        imm.close_luai()

        self.assertTrue("ASOUND TEST" in resp)

    def test_CH_method_zecho(self):
        self.test_CH_method_asound()

    def test_CH_method_kill(self):
        kill_mobs()
        imm.load_mob(test_config.MOB_GET_VNUM)
        
        val = get_luai_val("self.fighting == nil")
        self.assertTrue(val)

        script = """
            (function()
                getmobworld({})[1]:kill('{}')
                return self.fighting.vnum == {}
             end)()
        """.format(
                test_config.MOB_GET_VNUM,
                imm.name,
                test_config.MOB_GET_VNUM)

        val = get_luai_val(script)

        imm.send("peace")

        self.assertTrue(val)


