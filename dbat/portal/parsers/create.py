"""
The character creation parser.
"""

import dbat
from urllib.parse import quote
from .base import BaseParser
from dbat.portal.commands.base import CMD_MATCH
from httpx import HTTPStatusError
from dbat.utils import partial_match

from dbat.models.game import AccountData, PlayerData, ChargenData, HelpData
from dbat.models.names import Race, SubRace, Sensei, Mutation, BioGenome, Sex

class CreateParser(BaseParser):
    """
    Implements the character creation features.
    """

    def __init__(self):
        super().__init__()
        self.data = ChargenData()
    
    async def on_start(self):
        await self.handle_look()

    async def on_start(self):
        await self.handle_look()

    async def cghelp_name(self):
        await self.send_line("You must choose a name for your character. It should be unique and not already in use by another player.")

    async def cghelp_race(self):
        await self.send_line("You must choose a race for your character. Available races are: Human, Elf, Dwarf.")

    async def cghelp_subrace(self):
        await self.send_line("You must choose a subrace for your character. Available subraces are: Mountain Dwarf, High Elf.")

    async def cghelp_sex(self):
        await self.send_line("You must choose a sex for your character. Available options are: Male, Female.")

    async def cghelp_sensei(self):
        await self.send_line("You must choose a sensei for your character. Available senseis are: Master Roshi, Kakashi.")

    async def cghelp_mutation(self):
        await self.send_line("You must choose a mutation for your character. Available mutations are: Telepathy, Invisibility.")

    async def cghelp_bio(self):
        await self.send_line("You must provide a brief biography for your character.")

    async def cghelp_align(self):
        await self.send_line("You must choose an alignment for your character. Available alignments are: Good, Evil, Neutral.")

    async def handle_cghelp(self, args: str):
        if not args:
            await self.send_line("You must supply a topic for character creation help. Example: cghelp race")
            return
        topics = {"name", "race", "subrace", "sex", "sensei", "mutation", "bio", "align"}
        if not (choice := partial_match(args, topics, key=lambda t: t.name)):
            await self.send_line("Invalid topic for character creation help.")
            return
        await getattr(self, f"cghelp_{choice}")()

    async def handle_help(self, args: str):
        if not args:
            await self.send_line("You must supply a topic for general help. Example: help Saiyan")
            return
        try:
            help_data = await self.api_call("GET", f"/help/public/{quote(args)}")
        except HTTPStatusError as e:
            await self.send_line(f"Error retrieving help: {e.response.text}")
            return
        # The resulting body should be a HelpData...
        help_result = HelpData(**help_data)
        await self.send_line(help_result.index)
        await self.send_circle(help_result.entry)
    
    async def handle_name(self, args: str):
        if not args:
            await self.send_line("You must supply a name for your character.")
            return
        if " " in args:
            await self.send_line("Character names cannot contain spaces.")
            return
        if len(args) < 3:
            await self.send_line("Character names must be at least 3 characters long.")
            return
        if len(args) > 20:
            await self.send_line("Character names cannot exceed 20 characters.")
            return
        if not args.isalpha():
            await self.send_line("Character names can only contain alphabetic characters.")
            return
        self.data.name = args.capitalize()
        await self.send_line(f"Character name tentatively set to {self.data.name}.")
    
    async def handle_race(self, args: str):
        if not args:
            await self.send_line("You must supply a race for your character.")
            return
        if not (choice := partial_match(args, self.data.available_races(), key=lambda t: t.name)):
            await self.send_line("Invalid race choice. cghelp race for available options.")
            return
        if choice == self.data.race:
            await self.send_line(f"Character race is already set to {choice.name.capitalize()}.")
            return
        self.data.race = choice
        self.data.mutations.clear()
        self.data.bio_genomes.clear()
        if self.data.sensei not in self.data.available_senseis() and self.data.sensei is not None:
            self.data.sensei = None
            await self.send_line(f"Character sensei reset as your previous choice is no longer available.")
        if self.data.sex not in self.data.available_sexes() and self.data.sex is not None:
            self.data.sex = None
            await self.send_line(f"Character sex reset as your previous choice is no longer available.")
        await self.send_line(f"Character race set to: {self.data.race.name.capitalize()}.")
        match self.data.race:
            case Race.namekian:
                self.data.sex = Sex.neutral
                await self.send_line("Sex set to neutral as Namekians do not have a defined sex.")

    async def handle_subrace(self, args: str):
        if not (available := self.available_subraces()):
            await self.send_line("This race does not have subraces.")
            return
        if not args:
            await self.send_line("You must supply a subrace for your character.")
            return
        if not (choice := partial_match(args, available, key=lambda t: t.name)):
            await self.send_line("Invalid subrace choice. cghelp subrace for available options.")
            return
        if choice == self.data.subrace:
            await self.send_line(f"Character subrace is already set to {choice.name.capitalize()}.")
            return
        self.data.subrace = choice
        await self.send_line(f"Character subrace set to: {choice.name.capitalize()}.")

    async def handle_sex(self, args: str):
        if not (available := self.data.available_sexes()):
            await self.send_line("This race does not have a defined sex.")
            return
        if not args:
            await self.send_line("You must supply a sex for your character.")
            return
        if not (choice := partial_match(args, available, key=lambda t: t.name)):
            await self.send_line("Invalid sex choice. cghelp sex for available options.")
            return
        if choice == self.data.sex:
            await self.send_line(f"Character sex is already set to {choice.name.capitalize()}.")
            return
        self.data.sex = choice
        await self.send_line(f"Character sex set to: {choice.name.capitalize()}.")

    async def handle_sensei(self, args: str):
        if not (available := self.data.available_senseis()):
            await self.send_line("This race does not have a defined sensei.")
            return
        if not args:
            await self.send_line("You must supply a sensei for your character.")
            return
        if not (choice := partial_match(args, available, key=lambda t: t.name)):
            await self.send_line("Invalid sensei choice. cghelp sensei for available options.")
            return
        if choice == self.data.sensei:
            await self.send_line(f"Character sensei is already set to {choice.name.capitalize()}.")
            return
        self.data.sensei = choice
        await self.send_line(f"Character sensei set to: {choice.name.capitalize()}.")

    async def handle_mutation(self, args: str):
        if not self.data.race == Race.mutant:
            await self.send_line("Mutations are only available for the Mutant race.")
            return
        if not args:
            await self.send_line("You must supply a mutation for your character.")
            return
        if not (choice := partial_match(args, Mutation, key=lambda t: t.name)):
            await self.send_line("Invalid mutation choice. cghelp mutation for available options.")
            return
        if choice in self.data.mutations:
            self.data.mutations.remove(choice)
            await self.send_line(f"Character mutation {choice.name.capitalize()} removed.")
            return
        if len(self.data.mutations) >= 2:
            await self.send_line("You can only have 2 mutations. Remove one before adding another.")
            return
        self.data.mutations.add(choice)
        await self.send_line(f"Character mutation {choice.name.capitalize()} added.")

    async def handle_genome(self, args: str):
        if not self.data.race == Race.bio_android:
            await self.send_line("Bio Genomes are only available for the Bio Android race.")
            return
        if not args:
            await self.send_line("You must supply a bio genome for your character.")
            return
        if not (choice := partial_match(args, BioGenome, key=lambda t: t.name)):
            await self.send_line("Invalid bio genome choice. cghelp genome for available options.")
            return
        if choice in self.data.bio_genomes:
            self.data.bio_genomes.remove(choice)
            await self.send_line(f"Character bio genome {choice.name.capitalize()} removed.")
            return
        if len(self.data.bio_genomes) >= 2:
            await self.send_line("You can only have 2 bio genomes. Remove one before adding another.")
            return
        self.data.bio_genomes.add(choice)
        await self.send_line(f"Character bio genome {choice.name.capitalize()} added.")

    async def display_menu(self):
        help_table = self.make_table("Command", "Description", title="Character Creation Commands")
        help_table.add_row("cghelp <topic>", "E.g., cghelp race or cghelp align")
        help_table.add_row("help <topic>", "Read the MUD's helpfiles. E.g., help Saiyan, help kamehameha")
        help_table.add_row("choices", "Displays available choices for character creation.")
        help_table.add_row("name <name>", "Sets the character's name.")
        help_table.add_row("race <choice>", "Sets the character's race.")
        if self.data.race in ("android",):
            help_table.add_row("subrace <choice>", "Sets the character's subrace.")
        if self.data.race not in ("namekian",):
            help_table.add_row("sex <choice>", "Sets the character's sex.")
        help_table.add_row("sensei <choice>", "Sets the character's sensei.")
        if self.data.race == Race.mutant:
            help_table.add_row("mutation <choice>", "Adds a mutation to the character.")
        if self.data.race == Race.bio_android:
            help_table.add_row("genome <choice>", "Adds a bio genome to the character.")
        help_table.add_row("keep <yes|no>", "Whether to keep sensei skills.")
        help_table.add_row("look", "Lists the current character creation data.")
        help_table.add_row("align <n>", "-1000 max evil, -49 to 49 neutral, +1000 max good.")
        help_table.add_row("finish", "Submit form to save the character.")
        help_table.add_row("cancel", "Cancels the character creation.")
        await self.send_rich(help_table)
    
    async def handle_keep(self, args: str):
        if not args:
            await self.send_line("You must specify whether to keep sensei skills (yes/no).")
            return
        match args.lower():
            case "yes" | "y" | "1":
                self.data.keep_skills = True
                await self.send_line("Sensei skills will be kept.")
            case "no" | "n" | "0":
                self.data.keep_skills = False
                await self.send_line("Sensei skills will not be kept.")
            case _:
                await self.send_line("Invalid option. Use 'yes' or 'no'.")
    
    async def handle_align(self, args: str):
        if not args:
            await self.send_line("You must supply an alignment value for your character.")
            return
        try:
            align_value = int(args)
        except ValueError:
            await self.send_line("Alignment must be an integer between -1000 and 1000.")
            return
        if align_value < -1000 or align_value > 1000:
            await self.send_line("Alignment must be between -1000 and 1000.")
            return
        self.data.align = align_value
        await self.send_line(f"Character alignment set to: {self.data.align}.")

    async def handle_command(self, event: str):
        matched = CMD_MATCH.match(event)
        if not matched:
            await self.send_line("Invalid command. Type 'cghelp' for help.")
            return
        match_dict = {k: v for k, v in matched.groupdict().items() if v is not None}
        cmd = match_dict.get("cmd", "")
        args = match_dict.get("args", "").strip()
        lsargs = match_dict.get("lsargs", "")
        rsargs = match_dict.get("rsargs", "")
        match cmd.lower():
            case "cghelp":
                await self.handle_cghelp(args)
            case "help":
                await self.handle_help(args)
            case "name":
                await self.handle_name(args)
            case "race":
                await self.handle_race(args)
            case "subrace":
                await self.handle_subrace(args)
            case "sex":
                await self.handle_sex(args)
            case "sensei":
                await self.handle_sensei(args)
            case "mutation":
                await self.handle_mutation(args)
            case "genome":
                await self.handle_genome(args)
            case "keep":
                await self.handle_keep(args)
            case "align":
                await self.handle_align(args)
            case "finish":
                await self.handle_finish()
            case "cancel":
                await self.handle_cancel(args)
            case "look":
                await self.handle_look()
            case "choices":
                await self.display_choices()
            case _:
                await self.send_line("Invalid command. Type 'cghelp' for help.")

    async def display_choices(self):
        choices_table = self.make_table("Category", "Choices", title="Choices for Character Creation")
        choices_table.add_row("Race", ", ".join([e.name.capitalize() for e in self.data.available_races()]))
        if (subraces := self.data.available_subraces()):
            choices_table.add_row("SubRace", ", ".join([e.name.capitalize() for e in subraces]))
        if (sexes := self.data.available_sexes()):
            choices_table.add_row("Sex", ", ".join([e.name.capitalize() for e in sexes]))
        choices_table.add_row("Sensei", ", ".join([e.name.capitalize() for e in self.data.available_senseis()]))
        await self.send_rich(choices_table)

    async def display_data(self):
        data_table = self.make_table("Field", "Value", title="Character Creation Data")
        data_table.add_row("Name", self.data.name or "<unset>")
        data_table.add_row("Race", self.data.race.name.capitalize() if self.data.race else "<unset>")
        data_table.add_row("Sex", self.data.sex.name.capitalize() if self.data.sex else "<unset>")
        data_table.add_row("Sensei", self.data.sensei.name.capitalize() if self.data.sensei else "<unset>")
        match self.data.race:
            case Race.android:
                data_table.add_row("SubRace", self.data.subrace.name.capitalize() if self.data.subrace else "<unset>")
            case Race.mutant:
                data_table.add_row("Mutations", ", ".join([m.name.capitalize() for m in self.data.mutations]) or "<none>")
            case Race.bio_android:
                data_table.add_row("Bio Genomes", ", ".join([b.name.capitalize() for b in self.data.bio_genomes]) or "<none>")
        data_table.add_row("Keep Skills", "Yes" if self.data.keep_skills else "No")
        data_table.add_row("Alignment", str(self.data.align))
        
        await self.send_rich(data_table)

    async def handle_look(self):
        await self.display_data()
        await self.display_menu()

    async def handle_cancel(self):
        await self.connection.pop_parser()
    
    async def handle_finish(self):
        try:
            result = await self.api_call("POST", f"/characters/", json=self.data.model_dump())
        except HTTPStatusError as e:
            await self.send_line(f"Error creating character: {e.response.text}")
            return
        character = PlayerData(**result)
        await self.send_line(f"Character {character.name} created successfully!")
        await self.connection.pop_parser()