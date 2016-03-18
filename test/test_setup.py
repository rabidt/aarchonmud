import char
import test_config


imm = char.Immortal(test_config.IMM_NAME, test_config.IMM_PW)
imm.connect(test_config.GAME_HOST, test_config.GAME_IP)
