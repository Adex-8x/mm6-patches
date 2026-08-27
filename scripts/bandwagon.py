# this sucks, but it works
# intended with the context of MM6 is as follows:

# python3 bandwagon.py exploration_initial.nds mm6e.nds mm6c.nds c -a e -s SCRIPT_C/ -d SCRIPT_E/
# python3 bandwagon.py exploration_initial.nds merged.nds mm6o.nds o -s SCRIPT_O/

import os
from pathlib import Path
from colorama import Fore
from ndspy.fnt import Folder
from ordered_set import OrderedSet

import ndspy.code
from ndspy.rom import NintendoDSRom

from skytemple_files.common.util import (
	create_file_in_rom,
	folder_in_rom_exists,
	get_ppmdu_config_for_rom,
)

from skytemple_files.common.ppmdu_config.data import Pmd2Data
from skytemple_files.common.types.file_types import FileType
from skytemple_files.script.ssb.handler import SsbHandler
from skytemple_files.data.str.handler import StrHandler
from skytemple_files.script.ssa_sse_sss.handler import SsaHandler
from skytemple_files.graphics.bg_list_dat.handler import BgListDatHandler
from skytemple_files.common.ppmdu_config.script_data import Pmd2ScriptObject
from skytemple_files.graphics.kao.handler import KaoHandler
from skytemple_files.data.md.handler import MdHandler
from skytemple_files.data.waza_p.handler import WazaPHandler
from skytemple_files.graphics.kao import SUBENTRIES
from skytemple_files.data.data_cd.handler import DataCDHandler
from skytemple_files.hardcoded.monster_sprite_data_table import HardcodedMonsterGroundIdleAnimTable
from explorerscript.ssb_converting.ssb_data_types import SsbRoutineType

import re
import logging
import argparse

FILENAME_ACTOR_LIST = "BALANCE/actor_list.bin"
FILENAME_OBJECT_LIST = "BALANCE/objects.bin"
FILENAME_LEVEL_LIST = "BALANCE/level_list.bin"

FIRST_NEW_ACTOR = 69
FIRST_NEW_OBJECT = 558
FIRST_NEW_LEVEL = 435
FIRST_NEW_PROCESS = 64
FIRST_NEW_MONSTER_SPRITE = 600
FIRST_CUSTOM_MONSTER_ID = 1200
TEXT_STRING_PARTICIPANT_NAME_START = 8750-1
TEXT_STRING_COLLISION_RELOCATION = 9920-1 # Keep moving down...
NB_SCENES_PER_BRANCH = 11
NB_SCENES = NB_SCENES_PER_BRANCH*3 # 10 participants per 3 branches, plus 3 initials

SCRIPT_MYSTERY = "SCRIPT/MYSTERY/"
KAOMADO_KAO = "FONT/kaomado.kao"
MONSTER_MD = "BALANCE/monster.md"
BGLIST_FILE = "MAP_BG/bg_list.dat"
MONSTER_BIN = "MONSTER/monster.bin"
GROUND_BIN = "MONSTER/m_ground.bin"
ATTACK_BIN = "MONSTER/m_attack.bin"
DEFAULT_TEXT_STRINGS_FILE = "MESSAGE/text_e.str"
SP_EFFECTS = "BALANCE/process.bin"
WAZA_BINS = ["BALANCE/waza_p.bin", "BALANCE/waza_p2.bin"]

logging.basicConfig(
	format="%(asctime)s - %(name)s - %(levelname)s - %(message)s", level=logging.INFO
)

logger = logging.getLogger(__name__)
# Don't uncomment this lol some lists aren't mutable with it enabled :(
# change_implementation_type(ImplementationType.NATIVE)

SCRIPT_MYSTERY_PATTERN = re.compile(r"SCRIPT/MYSTERY/(\d{2}|initial)(\.ss[ab])")
SCRIPT_PATTERN = re.compile(r"(\d{2}|initial)(\.ss[ab])")

def create_folder_in_rom(rom: NintendoDSRom, path: str) -> None:
	"""Creates a folder in the ROM."""
	folder = rom.filenames.subfolder(path)
	if folder is not None:
		raise FileNotFoundError(f"Folder {path} already exists.")
	path_list = path.split("/")
	par_dir_name = "/".join(path_list[:-1])
	parent_dir: Folder | None = (
		rom.filenames.subfolder(par_dir_name) if len(path_list) > 1 else rom.filenames
	)
	if parent_dir is None:
		raise FileNotFoundError(f"Folder {par_dir_name} does not exist.")

	found = False
	first_id = -1
	last_child_count = -1
	for s_name, s_folder in sorted(parent_dir.folders, key=lambda f: f[0]):
		first_id = s_folder.firstID
		last_child_count = len(s_folder.files)
		if s_name > path_list[-1]:
			found = True
			break
	if not found:
		first_id = first_id + last_child_count

	new_folder = Folder(firstID=first_id)
	parent_dir.folders.append((path_list[-1], new_folder))

def delete_folder_in_rom(rom: NintendoDSRom, path: str) -> bool:
	"""Deletes a folder in the ROM, alongside all of its inner files and folders."""
	if not folder_in_rom_exists(rom, path):
		return False
	if path[0] == "/":
		path = path[1:]
	if path[-1] == "/":
		path = path[0:-1]
	path_list = path.split("/")
	current_name = path_list[-1]
	parent_folder = (
		rom.filenames.subfolder("/".join(path_list[:-1]))
		if len(path_list) > 1
		else rom.filenames
	)
	current_folder = rom.filenames.subfolder(path)
	for file in current_folder.files:
		delete_file_in_rom(rom, path + "/" + file)
	for folder in current_folder.folders:
		delete_folder_in_rom(rom, path + "/" + folder[0])
	for folder in parent_folder.folders:
		if folder[0] == current_name:
			parent_folder.folders.remove(folder)
			break
	return True

# great name i know
def copy_folder_from_rom2(dst: NintendoDSRom, src: NintendoDSRom, dst_path: str, src_path: str) -> None:
    """Copies a folder from one ROM to another, alongside all of its inner files and folders."""

    def recursive_folder_insertion(src_path: str, dst_path: str) -> None:
        create_folder_in_rom(dst, dst_path)
        folder = src.filenames.subfolder(src_path)
        for file in folder.files:
            filepath = dst_path + "/" + file
            create_file_in_rom(dst, filepath, src.getFileByName(src_path + "/" + file))
        for folder in folder.folders:
            recursive_folder_insertion(src_path + "/" + folder[0], dst_path + "/" + folder[0])

    recursive_folder_insertion(src_path, dst_path)

def rom_diff(old: NintendoDSRom, new: NintendoDSRom) -> dict:
    """Generates a dictionary diff between two ROMs' file systems."""
    diff = {
        "folders": {"added": [], "deleted": []},
        "files": {"added": [], "deleted": [], "changed": OrderedSet()},
    }

    def rom_diff_init(key: str):
        main = None
        other = None
        folder_list = None

        if key == "added":
            main = new
            other = old
        elif key == "deleted":
            main = old
            other = new
        else:
            return

        folder_list = main.filenames.folders

        for filepath in main.filenames.files:
            try:
                if other.getFileByName(filepath) != main.getFileByName(filepath):
                    diff["files"]["changed"].add(filepath)
            except ValueError:
                diff["files"][key].append(filepath)

        def recursive_rom_diff(folders: list, path: str):
            for name, folder in folders:
                current_path = path + "/" + name if len(path) > 0 else name
                files = folder.files
                other_folder: Folder | None = other.filenames.subfolder(current_path)
                if other_folder is not None:
                    for i, filename in enumerate(files):
                        filepath = current_path + "/" + filename
                        try:
                            if (
                                other.getFileByName(filepath)
                                != main.files[folder.firstID + i]
                            ):
                                diff["files"]["changed"].add(filepath)
                        except ValueError:
                            diff["files"][key].append(filepath)
                else:
                    diff["folders"][key].append(current_path)
                    for filename in files:
                        filepath = current_path + "/" + filename
                        diff["files"][key].append(filepath)
                recursive_rom_diff(folder.folders, current_path)

        recursive_rom_diff(folder_list, "")

    rom_diff_init("added")
    rom_diff_init("deleted")

    diff["folders"]["added"].reverse()

    return diff

def get_actor_list(rom: NintendoDSRom):
	return FileType.SIR0.unwrap_obj(
		FileType.SIR0.deserialize(rom.getFileByName(FILENAME_ACTOR_LIST)),
		FileType.ACTOR_LIST_BIN.type(),
	)


def set_actor_list(rom: NintendoDSRom, data) -> None:
	rom.setFileByName(
		FILENAME_ACTOR_LIST,
		FileType.SIR0.serialize(FileType.SIR0.wrap_obj(data)),
	)


def get_object_list(rom: NintendoDSRom):
	return FileType.OBJECT_LIST_BIN.deserialize(rom.getFileByName(FILENAME_OBJECT_LIST))


def set_object_list(rom: NintendoDSRom, data) -> None:
	rom.setFileByName(FILENAME_OBJECT_LIST, FileType.OBJECT_LIST_BIN.serialize(data))


def get_level_list(rom: NintendoDSRom):
	return FileType.SIR0.unwrap_obj(
		FileType.SIR0.deserialize(rom.getFileByName(FILENAME_LEVEL_LIST)),
		FileType.LEVEL_LIST_BIN.type(),
	)


def set_level_list(rom: NintendoDSRom, data) -> None:
	rom.setFileByName(
		FILENAME_LEVEL_LIST,
		FileType.SIR0.serialize(FileType.SIR0.wrap_obj(data)),
	)

def get_text_strings(rom: NintendoDSRom, path: str, config: Pmd2Data):
	return StrHandler.deserialize(
		rom.getFileByName(path), string_encoding=config.string_encoding
	)

def set_text_strings(rom: NintendoDSRom, path: str, data):
	rom.setFileByName(path, StrHandler.serialize(data))

def delete_file_in_rom(rom: NintendoDSRom, path: str) -> bool:
	"""Deletes a file in the ROM."""
	if path[0] == "/":
		path = path[1:]
	if path[-1] == "/":
		path = path[0:-1]
	path_list = path.split("/")
	dir_name = "/".join(path_list[:-1])
	file_name = path_list[-1]
	target_id = rom.filenames.idOf(path)
	folder = rom.filenames.subfolder(dir_name)
	if target_id is None or folder is None:
		return False
	folder.files.remove(file_name)

	def recursive_decrement_folder_start_idx(rfolder: Folder, target_idx: int) -> None:
		if rfolder != folder and rfolder.firstID >= target_idx:
			rfolder.firstID -= 1
		for __, sfolder in rfolder.folders:
			recursive_decrement_folder_start_idx(sfolder, target_idx)

	recursive_decrement_folder_start_idx(rom.filenames, target_id)
	rom.files.pop(target_id)
	# If the current directory is empty as a result of the deletion, delete the whole folder as well!
	if len(folder.files) == 0 and len(folder.folders) == 0:
		delete_folder_in_rom(rom, dir_name)
	return True


class SceneryWagon():
	def __init__(self):
		self.sprites = dict()
		self.portraits = dict()
		self.text_strings = dict()
		self.actors = dict()
		self.objects = dict()
		self.special_processes = dict()
		self.map_backgrounds = dict()
		self.levels = dict()
		self.monsters = dict()

	def get_sprite_models(self, rom: NintendoDSRom):
		pack_mon = FileType.BIN_PACK.deserialize(rom.getFileByName(MONSTER_BIN))
		pack_grd = FileType.BIN_PACK.deserialize(rom.getFileByName(GROUND_BIN))
		pack_atk = FileType.BIN_PACK.deserialize(rom.getFileByName(ATTACK_BIN))

		return (pack_mon,pack_grd,pack_atk,)

	def set_sprite_models(self, rom: NintendoDSRom, dst_mon, dst_grd, dst_atk):
		rom.setFileByName(MONSTER_BIN, FileType.BIN_PACK.serialize(dst_mon))
		rom.setFileByName(GROUND_BIN, FileType.BIN_PACK.serialize(dst_grd))
		rom.setFileByName(ATTACK_BIN, FileType.BIN_PACK.serialize(dst_atk))

	def add_to_datacd(self, dst: NintendoDSRom, src: NintendoDSRom, name: str, start: int, remapping: dict):
		previous_cd = DataCDHandler.deserialize(src.getFileByName(name))
		current_cd = DataCDHandler.deserialize(dst.getFileByName(name))
		for i in range(start, previous_cd.nb_items()):
			remapping[i] = current_cd.nb_items()
			effect_id = previous_cd.get_item_effect_id(i)
			current_cd.add_effect_code(previous_cd.get_effect_code(effect_id))
			current_cd.add_item_effect_id(current_cd.nb_effects()-1)

		dst.setFileByName(name, DataCDHandler.serialize(current_cd))
	
	def add_to_balance_list(self, current_data, previous_data, start, balance_remapping):
		extra = []
		for i in range(start, len(previous_data.list)):
			balance_remapping[i] = len(current_data.list)
			if start == FIRST_NEW_LEVEL:
				# Need to adjust the level's BG, always
				thingy = previous_data.list[i].mapid
				previous_data.list[i].mapid = self.map_backgrounds[thingy]
			elif start == FIRST_NEW_ACTOR:
				# Need to adjust the actor's kind...sometimes
				thingy = previous_data.list[i].entid
				if thingy in self.monsters.keys():
					previous_data.list[i].entid = self.monsters[thingy]
				else:
					# This is a vanilla monster that needs adjustment...
					extra.append(thingy)
					
				# Do they rely on a Text String?
				thingy = previous_data.list[i].unk3 - 1
				if thingy >= 0 and thingy in self.text_strings.keys():
					previous_data.list[i].unk3 = self.text_strings[thingy] + 1
				
			current_data.list.append(previous_data.list[i])

		return extra

	def update_sprites_portraits(self, dst: NintendoDSRom, src: NintendoDSRom, monster_list: list):
		pack_mon, pack_grd, pack_atk = self.get_sprite_models(src)
		dst_mon, dst_grd, dst_atk = self.get_sprite_models(dst)
		src_kao = KaoHandler.deserialize(src.getFileByName(KAOMADO_KAO))
		dst_kao = KaoHandler.deserialize(dst.getFileByName(KAOMADO_KAO))

		# sprite time
		md_bin = src.getFileByName(MONSTER_MD)
		md_model = MdHandler.deserialize(md_bin)
		entries = md_model.entries
		for i in monster_list:
			idx = entries[i].sprite_index
			dst_mon[idx] = pack_mon[idx]
			dst_grd[idx] = pack_grd[idx]
			dst_atk[idx] = pack_atk[idx]
		
		# portrait time
		for i in monster_list:
			for j in range(SUBENTRIES):
				img = src_kao.get(i-1, j)
				if img is not None: dst_kao.set(i-1, j, img)

		self.set_sprite_models(dst, dst_mon, dst_grd, dst_atk)
		dst.setFileByName(KAOMADO_KAO, KaoHandler.serialize(dst_kao))
		
	def transfer_sprites(self, dst: NintendoDSRom, src: NintendoDSRom):
		self.sprites = dict()
		md_bin = src.getFileByName(MONSTER_MD)
		md_model = MdHandler.deserialize(md_bin)
		entries = list(md_model.entries)
		pack_mon, pack_grd, pack_atk = self.get_sprite_models(src)
		dst_mon, dst_grd, dst_atk = self.get_sprite_models(dst)
		
		for entry in entries:
			if entry.sprite_index < FIRST_NEW_MONSTER_SPRITE:
				continue
			self.sprites[entry.sprite_index] = len(dst_mon)
			dst_mon.append(pack_mon[entry.sprite_index])
			dst_grd.append(pack_grd[entry.sprite_index])
			dst_atk.append(pack_atk[entry.sprite_index])

		self.set_sprite_models(dst, dst_mon, dst_grd, dst_atk)

	def transfer_monsters(self, dst: NintendoDSRom, src: NintendoDSRom, config):
		# sometimes i ask myself, how does technology even work
		self.monsters = dict()
		dst_model = MdHandler.deserialize(dst.getFileByName(MONSTER_MD))
		src_model = MdHandler.deserialize(src.getFileByName(MONSTER_MD))
		nb_entries = len(dst_model.entries)
		NAME_BLOCK = config.string_index_data.string_blocks["New Pokemon Names"]
		CATEGORY_BLOCK = config.string_index_data.string_blocks["New Pokemon Categories"]
		dst_strings = get_text_strings(dst, DEFAULT_TEXT_STRINGS_FILE, config)
		src_strings = get_text_strings(src, DEFAULT_TEXT_STRINGS_FILE, config)
		DST_MONSTER_NAMES = dst_strings.strings[NAME_BLOCK.begin : NAME_BLOCK.end]
		SRC_MONSTER_NAMES = src_strings.strings[NAME_BLOCK.begin : NAME_BLOCK.end]
		SRC_MONSTER_CATEGORIES = src_strings.strings[CATEGORY_BLOCK.begin : CATEGORY_BLOCK.end]
		src_kao = KaoHandler.deserialize(src.getFileByName(KAOMADO_KAO))
		dst_kao = KaoHandler.deserialize(dst.getFileByName(KAOMADO_KAO))
		dmy_idx = FIRST_CUSTOM_MONSTER_ID
		src_waza_models = [WazaPHandler.deserialize(src.getFileByName(waza_bin)) for waza_bin in WAZA_BINS]
		dst_waza_models = [WazaPHandler.deserialize(dst.getFileByName(waza_bin)) for waza_bin in WAZA_BINS]

		# surely sharing the config isn't a problem :clueless:
		overlays = dst.loadArm9Overlays()
		overlay11 = overlays[11]
		src_ground_anims = HardcodedMonsterGroundIdleAnimTable.get(src.loadArm9Overlays()[11].data, config)
		dst_ground_anims = HardcodedMonsterGroundIdleAnimTable.get(overlay11.data, config)
		
		for i,entry in enumerate(src_model.entries):
			if DST_MONSTER_NAMES[i] == SRC_MONSTER_NAMES[i] or (SRC_MONSTER_NAMES[i].startswith("DmyPk") or SRC_MONSTER_NAMES[i].startswith("reserve")):
				continue
			# Need to search for the next unused MdEntry in the dst model...
			can_replace = False
			while dmy_idx < nb_entries:
				if DST_MONSTER_NAMES[dmy_idx].startswith("DmyPk") and dst_model.entries[dmy_idx].sprite_index == 0:
					can_replace = True
					break
				dmy_idx += 1
			if can_replace:
				# Also adjust the sprite...
				remapped_id = self.sprites.get(entry.sprite_index)
				if remapped_id is not None:
					entry.sprite_index = remapped_id
				entry.entid = dmy_idx
				dst_model.entries[dmy_idx] = entry
				
				# And the text strings...
				dst_strings.strings[NAME_BLOCK.begin + dmy_idx] = SRC_MONSTER_NAMES[i]
				dst_strings.strings[CATEGORY_BLOCK.begin + dmy_idx] = SRC_MONSTER_CATEGORIES[i]
				
				# And the portraits...
				for j in range(SUBENTRIES):
					img = src_kao.get(i-1, j)
					if img is not None: dst_kao.set(dmy_idx-1, j, img)

				# And the moves...
				for k,model in enumerate(dst_waza_models):
					model.learnsets[dmy_idx] = src_waza_models[k].learnsets[i]

				# And the idle ground anim...
				# This is ONLY for when "ChangePokemonGroundAnim" is applied!
				dst_ground_anims[dmy_idx] = src_ground_anims[i]
				
				# ok we're done yay
				self.monsters[i] = dmy_idx
				dmy_idx += 1

		set_text_strings(dst, DEFAULT_TEXT_STRINGS_FILE, dst_strings)
		dst.setFileByName(KAOMADO_KAO, KaoHandler.serialize(dst_kao))
		dst.setFileByName(MONSTER_MD, MdHandler.serialize(dst_model))
		for i,filename in enumerate(WAZA_BINS):
			dst.setFileByName(filename, WazaPHandler.serialize(dst_waza_models[i]))
		HardcodedMonsterGroundIdleAnimTable.set(dst_ground_anims, overlay11.data, config)
		dst.files[overlay11.fileID] = overlay11.data
		dst.arm9OverlayTable = ndspy.code.saveOverlayTable(overlays)

	def transfer_mapbgs(self, dst: NintendoDSRom, src: NintendoDSRom):
		self.map_backgrounds = dict()
		src_bgs = BgListDatHandler.deserialize(src.getFileByName(BGLIST_FILE))
		dst_bgs = BgListDatHandler.deserialize(dst.getFileByName(BGLIST_FILE))
		for i, level in enumerate(src_bgs.level):
			if level.bma_name == dst_bgs.level[i].bma_name:
				continue
			self.map_backgrounds[i] = len(dst_bgs.level)
			dst_bgs.level.append(level)

		dst.setFileByName(BGLIST_FILE, BgListDatHandler.serialize(dst_bgs))

	def adjust_script_file(self, dst: NintendoDSRom, config, data: bytes, is_ssb: bool):
		if is_ssb:
			ssb = SsbHandler.deserialize(data, config)
			# Need to adjust all targeted routines to their proper actor/object...
			for i,routine_info in enumerate(ssb.routine_info):
				new_id = None
				
				nb_bytes,routine = routine_info
				if routine.type == SsbRoutineType.ACTOR:
					new_id = self.actors.get(routine.linked_to)
				elif routine.type == SsbRoutineType.OBJECT:
					new_id = self.objects.get(routine.linked_to)
				
				if new_id:
					routine.linked_to = new_id
				
				# OK, done with changes!
				ssb.routine_info[i] = (nb_bytes,routine,)
			
			# Now, the opcodes that reference an Actor, Object, or Level...
			for i,routine_ops in enumerate(ssb.routine_ops):
				for j,ssb_operation in enumerate(routine_ops):
					if ssb_operation.op_code.id == 0xCC: # ProcessSpecial
						new_param = self.special_processes.get(ssb_operation.params[0])
						if new_param:
							ssb_operation.params[0] = new_param
					else:
						for k,arg in enumerate(ssb_operation.op_code.arguments):
							new_param = None
							if arg.type == "Entity" or (ssb_operation.op_code.id == 0x10A and k == 0): # SetPositionLives
								new_param = self.actors.get(ssb_operation.params[k])
							elif arg.type == "Object":
								new_param = self.objects.get(ssb_operation.params[k])
							elif arg.type == "Level":
								new_param = self.levels.get(ssb_operation.params[k])
							
							if new_param:
								ssb_operation.params[k] = new_param
					
			data = SsbHandler.serialize(ssb, config)
		else:
			ssa = SsaHandler.deserialize(data)
			actors = get_actor_list(dst).list
			objects = get_object_list(dst).list
			# Need to adjust the SSAs to have their proper Actors and Objects...
			for i,layer in enumerate(ssa.layer_list):
				for j,ssa_actor in enumerate(layer.actors):
					new_id = self.actors.get(ssa_actor.actor.id)
					if new_id:
						ssa.layer_list[i].actors[j].actor = actors[new_id]

				for j,ssa_object in enumerate(layer.objects):
					new_id = self.objects.get(ssa_object.object.id)
					if new_id:
						ssa.layer_list[i].objects[j].object = objects[new_id]

			data = SsaHandler.serialize(ssa)
		return data

	def transfer_text_strings(self, dst: NintendoDSRom, src: NintendoDSRom, base: str):
		self.text_strings = dict()
		base = NintendoDSRom.fromFile(base)
		base_strings = get_text_strings(base, DEFAULT_TEXT_STRINGS_FILE, get_ppmdu_config_for_rom(base))
		
		def compare_text_strings(new_strings):
			changed = []
			for i,base_str in enumerate(base_strings.strings):
				if base_str != new_strings.strings[i]:
					changed.append(i)
			return changed

		dst_strings = get_text_strings(dst, DEFAULT_TEXT_STRINGS_FILE, get_ppmdu_config_for_rom(dst))
		src_strings = get_text_strings(src, DEFAULT_TEXT_STRINGS_FILE, get_ppmdu_config_for_rom(src))
		dst_modified_strings = compare_text_strings(dst_strings)
		src_modified_strings = compare_text_strings(src_strings)
		config = get_ppmdu_config_for_rom(dst)
		NAME_BLOCK = config.string_index_data.string_blocks["New Pokemon Names"]
		CATEGORY_BLOCK = config.string_index_data.string_blocks["New Pokemon Categories"]
		collision_relocation = TEXT_STRING_COLLISION_RELOCATION
		
		try:
			# to force diffs on things that wouldn't be caught
			# this is because when comparing between the "initial" ROM and the modified source,
			# all initial actors' Text String names wouldn't be caught otherwise.
			with open("text_strings.txt", "r") as f:
				stuff = f.read().split("\n")
			gaming = []
			for bepis in stuff:
				try:
					gaming.append(int(bepis)-1)
				except ValueError:
					pass
			dst_modified_strings += gaming
		except Exception:
			pass

		colliding_strings = []
		for i in src_modified_strings:
			if i in range(NAME_BLOCK.begin, NAME_BLOCK.end) or i in range(CATEGORY_BLOCK.begin, CATEGORY_BLOCK.end) or i in range(TEXT_STRING_PARTICIPANT_NAME_START, TEXT_STRING_PARTICIPANT_NAME_START+NB_SCENES):
				continue
			
			if i in dst_modified_strings:
				# reorg based on blank string
				colliding_strings.append(i)
				# Find next blank string moving down...
				while collision_relocation >= 0:
					if dst_strings.strings[collision_relocation] == "":
						dst_strings.strings[collision_relocation] = src_strings.strings[i]
						self.text_strings[i] = collision_relocation
						collision_relocation -= 1
						break
					collision_relocation -= 1
			else:
				dst_strings.strings[i] = src_strings.strings[i]

		if len(colliding_strings) > 0:
			logger.info(f"{Fore.LIGHTYELLOW_EX}The following strings collide across branches: {colliding_strings}{Fore.RESET}")
		else:
			logger.info(f"{Fore.LIGHTGREEN_EX}No strings collide!{Fore.RESET}")

		# Need to transfer over the new participant names in a certain spot...
		scene_mult = 1 
		new_participant_name_start = None
		while True:
			new_participant_name_start = TEXT_STRING_PARTICIPANT_NAME_START+(NB_SCENES_PER_BRANCH*scene_mult)
			if len(dst_strings.strings[new_participant_name_start]) > 0:
				scene_mult += 1
			else:
				break
		dst_strings.strings[new_participant_name_start:new_participant_name_start+NB_SCENES_PER_BRANCH] = src_strings.strings[TEXT_STRING_PARTICIPANT_NAME_START:TEXT_STRING_PARTICIPANT_NAME_START+NB_SCENES_PER_BRANCH]
		

		set_text_strings(dst, DEFAULT_TEXT_STRINGS_FILE, dst_strings)

def main():
	parser = argparse.ArgumentParser(
		prog="Bandwagon",
		description="A tool to merge two MysteryMail 6 ROMs.",
	)
	parser.add_argument("base", help="ROM base of the destination, to compare against text strings")
	parser.add_argument("dst", help="ROM used as the destination, to add files into")
	parser.add_argument("src", help="ROM used as the source, to extract files from")
	parser.add_argument("suffix", help="letter suffix (e, c, o)")
	parser.add_argument(
        "-o",
        "--output",
        help="ROM output file",
    )
	parser.add_argument(
		"-a",
		"--adjust",
		help="adjust dst scripts and BGMs with a new suffix (e, c, o)",
	)
	parser.add_argument(
		"-s",
		"--srcdir",
		help="define a SCRIPT directory rather than using the src ROM",
	)
	parser.add_argument(
		"-d",
		"--dstdir",
		help="define a SCRIPT directory rather than using the dst ROM",
	)
	args = parser.parse_args()
	out = "merged.nds" if not args.output else args.output
	wagon = SceneryWagon()
	
	dst = NintendoDSRom.fromFile(args.dst)
	src = NintendoDSRom.fromFile(args.src)
	diff = rom_diff(dst, src)
	config = get_ppmdu_config_for_rom(dst)
	
	# Create a new BGM folder from src to dst...
	copy_folder_from_rom2(dst, src, f"SOUND/BGM{args.suffix.upper()}", "SOUND/BGM")
	# Transfer all new sprites over...
	wagon.transfer_sprites(dst, src)
	# Next, we need to adjust the Pokémon...
	wagon.transfer_monsters(dst, src, config)
	
	# Now, the Map Backgrounds...
	wagon.transfer_mapbgs(dst, src)

	# Now, the Special Processes that are ASM and not in C...
	wagon.add_to_datacd(dst, src, SP_EFFECTS, FIRST_NEW_PROCESS, wagon.special_processes)
	
	# File loop time yay
	for target in diff["files"]["added"]:
		if target.startswith("SOUND/BGM/") or target.startswith(SCRIPT_MYSTERY):
			continue
		srcfile = src.getFileByName(target)
		create_file_in_rom(dst, target, srcfile)

	# adjust dst scripts...but from a directory
	if args.dstdir:
		for file in Path(args.dstdir).glob("*.ssb"):
			with file.open("rb") as f:
				data = f.read()
			dst.setFileByName(f"{SCRIPT_MYSTERY}{file.name}", data)

	# adjust dst scripts/BGM if -a flag set
	if args.adjust:
		suffix = args.adjust.lower()
		folder = dst.filenames.subfolder("SCRIPT/MYSTERY")
		for i,file in enumerate(folder.files):
			result = SCRIPT_PATTERN.match(file)
			if not result:
				continue
			folder.files[i] = result.group(1) + suffix + result.group(2)
			
		copy_folder_from_rom2(dst, dst, f"SOUND/BGM{args.adjust.upper()}", "SOUND/BGM")
		delete_folder_in_rom(dst, "SOUND/BGM")

	# Text Strings
	wagon.transfer_text_strings(dst, src, args.base)

	# File-specific stuff
	data = get_actor_list(dst)
	affected_vanilla_monsters = wagon.add_to_balance_list(data, get_actor_list(src), FIRST_NEW_ACTOR, wagon.actors)
	# logger.info(f"{Fore.LIGHTYELLOW_EX}Actor Remapping: {wagon.actors}{Fore.RESET}")
	set_actor_list(dst, data)
	wagon.update_sprites_portraits(dst, src, affected_vanilla_monsters)
	data = get_object_list(dst)
	wagon.add_to_balance_list(data, get_object_list(src), FIRST_NEW_OBJECT, wagon.objects)
	set_object_list(dst, data)
	data = get_level_list(dst)
	wagon.add_to_balance_list(data, get_level_list(src), FIRST_NEW_LEVEL, wagon.levels)
	set_level_list(dst, data)

	# SSA/SSB time
	if args.srcdir:
		# dude someday i really need to just commit to using pathlib idk why i'm lazy and insist on os
		files = os.listdir(args.srcdir)
		stuff = []
		for f in files:
			stuff.append(os.path.join(args.srcdir, f"{SCRIPT_MYSTERY}{f}"))
			
	else:
		stuff = list(diff["files"]["changed"]) + list(diff["files"]["added"])
		
	for target in stuff:
		# Check for script that has to be modified slightly
		result = SCRIPT_MYSTERY_PATTERN.search(target)
		if result is not None:
			if args.srcdir:
				with open(target.replace(SCRIPT_MYSTERY, ""), "rb") as f:
					srcfile = f.read()
			else:
				srcfile = src.getFileByName(target)
			name = f"{SCRIPT_MYSTERY}{result.group(1)}{args.suffix.lower()}{result.group(2)}"
			new_data = wagon.adjust_script_file(dst, config, srcfile, target[-1] == 'b')
			create_file_in_rom(dst, name, new_data)
			
	dst.saveToFile(out)
	logger.info(f"{Fore.LIGHTGREEN_EX}OK!{Fore.RESET}")

if __name__ == "__main__":
	main()
