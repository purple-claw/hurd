/*
 * Mach Operating System
 * Copyright (c) 1992,1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * Bootstrap the various built-in servers.
 */

#include <mach.h>
#include <mach/message.h>

#include <file_io.h>

#include <stdio.h>

#include "../boot/boot_script.h"
#include "translate_root.h"

#if 0
/*
 *	Use 8 Kbyte stacks instead of the default 64K.
 *	Use 4 Kbyte waiting stacks instead of the default 8K.
 */
#if	defined(alpha)
vm_size_t	cthread_stack_size = 16 * 1024;
#else
vm_size_t	cthread_stack_size = 8 * 1024;
#endif
#endif

extern
vm_size_t	cthread_wait_stack_size;

mach_port_t	bootstrap_master_device_port;	/* local name */
mach_port_t	bootstrap_master_host_port;	/* local name */

int	boot_load_program();

char	*root_name;
char	boot_script_name[MAXPATHLEN];

extern void	default_pager();
extern void	default_pager_initialize();
extern void	default_pager_setup();

/* initialized in default_pager_initialize */
extern mach_port_t default_pager_exception_port;
extern mach_port_t default_pager_bootstrap_port;

/*
 * Convert ASCII to integer.
 */
int atoi(str)
	register const char *str;
{
	register int	n;
	register int	c;
	int	is_negative = 0;

	n = 0;
	while (*str == ' ')
	    str++;
	if (*str == '-') {
	    is_negative = 1;
	    str++;
	}
	while ((c = *str++) >= '0' && c <= '9') {
	    n = n * 10 + (c - '0');
	}
	if (is_negative)
	    n = -n;
	return (n);
}

__main ()
{
}

static void
boot_panic (kern_return_t err)
{
#define PFX "bootstrap: "
  char *err_string = boot_script_error_string (err);
  char panic_string[strlen (err_string) + sizeof (PFX)];
  strcpy (panic_string, PFX);
  strcat (panic_string, err_string);
  panic (panic_string);
#undef PFX
}

void
safe_gets (char *str, int maxlen)
{
  char *c;
  c = index (fgets (str, maxlen, stdin), '\n');
  *c = '\0';
}

printf_init (device_t master)
{
  mach_port_t cons;
  device_open (master, D_READ|D_WRITE, "console", &cons);
  stdin = mach_open_devstream (cons, "r");
  stdout = stderr = mach_open_devstream (cons, "w");
  mach_port_deallocate (mach_task_self (), cons);
  setbuf (stdout, 0);
}

/*
 * Bootstrap task.
 * Runs in user spacep.
 *
 * Called as 'boot -switches host_port device_port root_name'
 *
 */
main(argc, argv)
	int	argc;
	char	**argv;
{
  int doing_default_pager = 0;
  int script_paging_file (const struct cmd *cmd, int *val)
    {
      if (add_paging_file (bootstrap_master_device_port, cmd->path))
	{
	  printf ("(bootstrap): %s: Cannot add paging file\n", cmd->path);
	  return BOOT_SCRIPT_MACH_ERROR;
	}
      return 0;
    }
  int script_default_pager (const struct cmd *cmd, int *val)
    {
      default_pager_initialize(bootstrap_master_host_port);
      doing_default_pager = 1;
      return 0;
    }

  register kern_return_t	result;
  struct file scriptf;

	task_t			my_task = mach_task_self();

	char			*flag_string;

	boolean_t		ask_boot_script = 0;

	static char		new_root[16];

	/*
	 * Use 4Kbyte cthread wait stacks.
	 */
	cthread_wait_stack_size = 4 * 1024;

	/*
	 * Parse the arguments.
	 */
	if (argc < 5)
	    panic("bootstrap: not enough arguments");

	/*
	 * Arg 0 is program name
	 */

	/*
	 * Arg 1 is flags
	 */
	if (argv[1][0] != '-')
	    panic("bootstrap: no flags");

	flag_string = argv[1];

	/*
	 * Arg 2 is host port number
	 */
	bootstrap_master_host_port = atoi(argv[2]);

	/*
	 * Arg 3 is device port number
	 */
	bootstrap_master_device_port = atoi(argv[3]);

	/*
	 * Arg 4 is root name
	 */
	root_name = argv[4];

	printf_init(bootstrap_master_device_port);
#ifdef pleasenoXXX
	panic_init(bootstrap_master_host_port);
#endif

	if (root_name[0] == '\0')
                root_name = DEFAULT_ROOT;

	/*
	 * If the '-a' (ask) switch was specified, ask for
	 * the root device.
	 */

	if (index(flag_string, 'a')) {
		printf("root device? [%s] ", root_name);
                safe_gets(new_root, sizeof(new_root));
	}

	if (new_root[0] == '\0')
		strcpy(new_root, root_name);

	root_name = translate_root(new_root);

	(void) strbuild(boot_script_name,
			"/dev/",
			root_name,
			"/boot/servers.boot",
			(char *)0);

	/*
	 * If the '-q' (query) switch was specified, ask for the
	 * server boot script.
	 */

	if (index(flag_string, 'q'))
	    ask_boot_script = TRUE;

	while (TRUE) {
	    if (ask_boot_script) {
		char new_boot_script[MAXPATHLEN];

		printf("Server boot script? [%s] ", boot_script_name);
		safe_gets(new_boot_script, sizeof(new_boot_script));
		if (new_boot_script[0] != '\0')
		    strcpy(boot_script_name, new_boot_script);
	    }

	    result = open_file(bootstrap_master_device_port,
			       boot_script_name,
			       &scriptf);
	    if (result != 0) {
		printf("Can't open server boot script %s: %d\n",
			boot_script_name,
			result);
		ask_boot_script = TRUE;
		continue;
	    }
	    break;
	}
	
	/*
	 * If the server boot script name was changed,
	 * then use the new device name as the root device.
	 */
	{
		char *dev, *end;
		int len;

		dev = boot_script_name;
		if (strncmp(dev, "/dev/", 5) == 0)
			dev += 5;
		end = strchr(dev, '/');
		len = end ? end-dev : strlen(dev);
		memcpy(root_name, dev, len);
		root_name[len] = 0;
	}

	/*
	 * Set up the default pager.
	 */
	partition_init();

	{
	  /* Initialize boot script variables.  */
	  if (boot_script_set_variable ("host-port", VAL_PORT,
					(int) bootstrap_master_host_port)
	      || boot_script_set_variable ("device-port", VAL_PORT,
					   (int) bootstrap_master_device_port)
	      || boot_script_set_variable ("root-device", VAL_STR,
					   (int) root_name)
	      || boot_script_set_variable ("boot-args", VAL_STR,
					   (int) flag_string)
	      || boot_script_define_function ("add-paging-file", VAL_NONE,
					      &script_paging_file)
	      || boot_script_define_function ("default-pager", VAL_NONE,
					      &script_default_pager)
	      )
	    panic ("bootstrap: error setting boot script variables");

	  parse_script (&scriptf);
	  close_file (&scriptf);
	}

	if (index (flag_string, 'd'))
	  {
	    char c;
	    printf ("Hit return to boot...");
	    safe_gets (&c, 1);
	  }

	/*
	 * task_set_exception_port and task_set_bootstrap_port
	 * both require a send right.
	 */
	(void) mach_port_insert_right(my_task, default_pager_bootstrap_port,
				      default_pager_bootstrap_port,
				      MACH_MSG_TYPE_MAKE_SEND);
	(void) mach_port_insert_right(my_task, default_pager_exception_port,
				      default_pager_exception_port,
				      MACH_MSG_TYPE_MAKE_SEND);

	/*
	 * Change our exception port.
	 */
	(void) task_set_exception_port(my_task, default_pager_exception_port);

	result = boot_script_exec ();

	if (result)
	  boot_panic (result);

#if 0
	{
	    /*
	     * Delete the old stack (containing only the arguments).
	     */
	    vm_offset_t	addr = (vm_offset_t) argv;

	    vm_offset_t		r_addr;
	    vm_size_t		r_size;
	    vm_prot_t		r_protection, r_max_protection;
	    vm_inherit_t	r_inheritance;
	    boolean_t		r_is_shared;
	    memory_object_name_t r_object_name;
	    vm_offset_t		r_offset;
	    kern_return_t	kr;

	    r_addr = addr;

	    kr = vm_region(my_task,
			&r_addr,
			&r_size,
			&r_protection,
			&r_max_protection,
			&r_inheritance,
			&r_is_shared,
			&r_object_name,
			&r_offset);
	    if ((kr == KERN_SUCCESS) && MACH_PORT_VALID(r_object_name))
		(void) mach_port_deallocate(my_task, r_object_name);
	    if ((kr == KERN_SUCCESS) &&
		(r_addr <= addr) &&
		((r_protection & (VM_PROT_READ|VM_PROT_WRITE)) ==
					(VM_PROT_READ|VM_PROT_WRITE)))
		(void) vm_deallocate(my_task, r_addr, r_size);
	}
#endif

	if (! doing_default_pager)
	  task_terminate (mach_task_self ());

	/*
	 * Become the default pager
	 */
	default_pager();
	/*NOTREACHED*/
}

/* Parse the boot script.  */
parse_script (struct file *f)
{
  char *p, *line, *buf;
  int amt, fd, err;
  int n = 0;

  buf = malloc (f->f_size);
  if (read_file (f, 0, buf, f->f_size, 0))
    panic ("bootstrap: error reading boot script file");

  line = p = buf;
  while (1)
    {
      while (p < buf + f->f_size && *p != '\n')
	  p++;
      *p = '\0';
      err = boot_script_parse_line (line);
      if (err)
	boot_panic (err);
      if (p == buf + f->f_size)
	break;
      line = ++p;

    }
}
