#! /usr/bin/env python
# encoding: utf-8
import sys
if sys.hexversion < 0x020400f0: from sets import Set as set
import os,sys,fnmatch,re,stat
import Utils,Constants
UNDEFINED=0
DIR=1
FILE=2
BUILD=3
type_to_string={UNDEFINED:"unk",DIR:"dir",FILE:"src",BUILD:"bld"}
prune_pats='.git .bzr .hg .svn _MTN _darcs CVS SCCS'.split()
exclude_pats=prune_pats+'*~ #*# .#* %*% ._* .gitignore .cvsignore vssver.scc .DS_Store'.split()
exclude_regs='''
**/*~
**/#*#
**/.#*
**/%*%
**/._*
**/CVS
**/CVS/**
**/.cvsignore
**/SCCS
**/SCCS/**
**/vssver.scc
**/.svn
**/.svn/**
**/.git
**/.git/**
**/.gitignore
**/.bzr
**/.bzr/**
**/.hg
**/.hg/**
**/_MTN
**/_MTN/**
**/_darcs
**/_darcs/**
**/.DS_Store'''
class Node(object):
	__slots__=("name","parent","id","childs")
	def __init__(self,name,parent,node_type=UNDEFINED):
		self.name=name
		self.parent=parent
		self.__class__.bld.id_nodes+=4
		self.id=self.__class__.bld.id_nodes+node_type
		if node_type==DIR:self.childs={}
		if parent and name in parent.childs:
			raise Utils.WafError('node %s exists in the parent files %r already'%(name,parent))
		if parent:parent.childs[name]=self
	def __setstate__(self,data):
		if len(data)==4:
			(self.parent,self.name,self.id,self.childs)=data
		else:
			(self.parent,self.name,self.id)=data
	def __getstate__(self):
		if getattr(self,'childs',None)is None:
			return(self.parent,self.name,self.id)
		else:
			return(self.parent,self.name,self.id,self.childs)
	def __str__(self):
		if not self.parent:return''
		return"%s://%s"%(type_to_string[self.id&3],self.abspath())
	def __repr__(self):
		return self.__str__()
	def __hash__(self):
		raise Utils.WafError('nodes, you are doing it wrong')
	def __copy__(self):
		raise Utils.WafError('nodes are not supposed to be cloned')
	def get_type(self):
		return self.id&3
	def set_type(self,t):
		self.id=self.id+t-self.id&3
	def dirs(self):
		return[x for x in self.childs.values()if x.id&3==DIR]
	def files(self):
		return[x for x in self.childs.values()if x.id&3==FILE]
	def get_dir(self,name,default=None):
		node=self.childs.get(name,None)
		if not node or node.id&3!=DIR:return default
		return node
	def get_file(self,name,default=None):
		node=self.childs.get(name,None)
		if not node or node.id&3!=FILE:return default
		return node
	def get_build(self,name,default=None):
		node=self.childs.get(name,None)
		if not node or node.id&3!=BUILD:return default
		return node
	def find_resource(self,lst):
		if isinstance(lst,str):
			lst=Utils.split_path(lst)
		if len(lst)==1:
			parent=self
		else:
			parent=self.find_dir(lst[:-1])
			if not parent:return None
		self.__class__.bld.rescan(parent)
		name=lst[-1]
		node=parent.childs.get(name,None)
		if node:
			tp=node.id&3
			if tp==FILE or tp==BUILD:
				return node
			else:
				return None
		tree=self.__class__.bld
		if not name in tree.cache_dir_contents[parent.id]:
			return None
		path=parent.abspath()+os.sep+name
		try:
			st=Utils.h_file(path)
		except IOError:
			return None
		child=self.__class__(name,parent,FILE)
		tree.node_sigs[0][child.id]=st
		return child
	def find_or_declare(self,lst):
		if isinstance(lst,str):
			lst=Utils.split_path(lst)
		if len(lst)==1:
			parent=self
		else:
			parent=self.find_dir(lst[:-1])
			if not parent:return None
		self.__class__.bld.rescan(parent)
		name=lst[-1]
		node=parent.childs.get(name,None)
		if node:
			tp=node.id&3
			if tp!=BUILD:
				raise Utils.WafError('find_or_declare cannot return a build node (build files in the source directory %r?)'%lst)
			return node
		node=self.__class__(name,parent,BUILD)
		return node
	def find_dir(self,lst):
		if isinstance(lst,str):
			lst=Utils.split_path(lst)
		current=self
		for name in lst:
			self.__class__.bld.rescan(current)
			prev=current
			if not current.parent and name==current.name:
				continue
			elif not name:
				continue
			elif name=='.':
				continue
			elif name=='..':
				current=current.parent or current
			else:
				current=prev.childs.get(name,None)
				if current is None:
					dir_cont=self.__class__.bld.cache_dir_contents
					if prev.id in dir_cont and name in dir_cont[prev.id]:
						if not prev.name:
							if os.sep=='/':
								dirname=os.sep+name
							else:
								dirname=name
						else:
							dirname=prev.abspath()+os.sep+name
						if not os.path.isdir(dirname):
							return None
						current=self.__class__(name,prev,DIR)
					elif(not prev.name and len(name)==2 and name[1]==':')or name.startswith('\\\\'):
						current=self.__class__(name,prev,DIR)
					else:
						return None
				else:
					if current.id&3!=DIR:
						return None
		return current
	def ensure_dir_node_from_path(self,lst):
		if isinstance(lst,str):
			lst=Utils.split_path(lst)
		current=self
		for name in lst:
			if not name:
				continue
			elif name=='.':
				continue
			elif name=='..':
				current=current.parent or current
			else:
				prev=current
				current=prev.childs.get(name,None)
				if current is None:
					current=self.__class__(name,prev,DIR)
		return current
	def exclusive_build_node(self,path):
		lst=Utils.split_path(path)
		name=lst[-1]
		if len(lst)>1:
			parent=None
			try:
				parent=self.find_dir(lst[:-1])
			except OSError:
				pass
			if not parent:
				parent=self.ensure_dir_node_from_path(lst[:-1])
				self.__class__.bld.rescan(parent)
			else:
				try:
					self.__class__.bld.rescan(parent)
				except OSError:
					pass
		else:
			parent=self
		node=parent.childs.get(name,None)
		if not node:
			node=self.__class__(name,parent,BUILD)
		return node
	def path_to_parent(self,parent):
		lst=[]
		p=self
		h1=parent.height()
		h2=p.height()
		while h2>h1:
			h2-=1
			lst.append(p.name)
			p=p.parent
		if lst:
			lst.reverse()
			ret=os.path.join(*lst)
		else:
			ret=''
		return ret
	def find_ancestor(self,node):
		dist=self.height()-node.height()
		if dist<0:return node.find_ancestor(self)
		cand=self
		while dist>0:
			cand=cand.parent
			dist-=1
		if cand==node:return cand
		cursor=node
		while cand.parent:
			cand=cand.parent
			cursor=cursor.parent
			if cand==cursor:return cand
	def relpath_gen(self,from_node):
		if self==from_node:return'.'
		if from_node.parent==self:return'..'
		ancestor=self.find_ancestor(from_node)
		lst=[]
		cand=self
		while not cand.id==ancestor.id:
			lst.append(cand.name)
			cand=cand.parent
		cand=from_node
		while not cand.id==ancestor.id:
			lst.append('..')
			cand=cand.parent
		lst.reverse()
		return os.sep.join(lst)
	def nice_path(self,env=None):
		tree=self.__class__.bld
		ln=tree.launch_node()
		if self.id&3==FILE:return self.relpath_gen(ln)
		else:return os.path.join(tree.bldnode.relpath_gen(ln),env.variant(),self.relpath_gen(tree.srcnode))
	def is_child_of(self,node):
		p=self
		diff=self.height()-node.height()
		while diff>0:
			diff-=1
			p=p.parent
		return p.id==node.id
	def variant(self,env):
		if not env:return 0
		elif self.id&3==FILE:return 0
		else:return env.variant()
	def height(self):
		d=self
		val=-1
		while d:
			d=d.parent
			val+=1
		return val
	def abspath(self,env=None):
		variant=(env and(self.id&3!=FILE)and env.variant())or 0
		ret=self.__class__.bld.cache_node_abspath[variant].get(self.id,None)
		if ret:return ret
		if not variant:
			if not self.parent:
				val=os.sep=='/'and os.sep or''
			elif not self.parent.name:
				val=(os.sep=='/'and os.sep or'')+self.name
			else:
				val=self.parent.abspath()+os.sep+self.name
		else:
			val=os.sep.join((self.__class__.bld.bldnode.abspath(),variant,self.path_to_parent(self.__class__.bld.srcnode)))
		self.__class__.bld.cache_node_abspath[variant][self.id]=val
		return val
	def change_ext(self,ext):
		name=self.name
		k=name.rfind('.')
		if k>=0:
			name=name[:k]+ext
		else:
			name=name+ext
		return self.parent.find_or_declare([name])
	def src_dir(self,env):
		return self.parent.srcpath(env)
	def bld_dir(self,env):
		return self.parent.bldpath(env)
	def bld_base(self,env):
		s=os.path.splitext(self.name)[0]
		return os.path.join(self.bld_dir(env),s)
	def bldpath(self,env=None):
		if self.id&3==FILE:
			return self.relpath_gen(self.__class__.bld.bldnode)
		p=self.path_to_parent(self.__class__.bld.srcnode)
		if p != '':
			return env.variant()+os.sep+p
		return env.variant()
	def srcpath(self,env=None):
		if self.id&3==BUILD:
			return self.bldpath(env)
		return self.relpath_gen(self.__class__.bld.bldnode)
	def read(self,env):
		return Utils.readf(self.abspath(env))
	def dir(self,env):
		return self.parent.abspath(env)
	def file(self):
		return self.name
	def file_base(self):
		return os.path.splitext(self.name)[0]
	def suffix(self):
		k=max(0,self.name.rfind('.'))
		return self.name[k:]
	def find_iter_impl(self,src=True,bld=True,dir=True,accept_name=None,is_prune=None,maxdepth=25):
		bld_ctx=self.__class__.bld
		bld_ctx.rescan(self)
		for name in bld_ctx.cache_dir_contents[self.id]:
			if accept_name(self,name):
				node=self.find_resource(name)
				if node:
					if src and node.id&3==FILE:
						yield node
				else:
					node=self.find_dir(name)
					if node and node.id!=bld_ctx.bldnode.id:
						if dir:
							yield node
						if not is_prune(self,name):
							if maxdepth:
								for k in node.find_iter_impl(src,bld,dir,accept_name,is_prune,maxdepth=maxdepth-1):
									yield k
			else:
				if not is_prune(self,name):
					node=self.find_resource(name)
					if not node:
						node=self.find_dir(name)
						if node and node.id!=bld_ctx.bldnode.id:
							if maxdepth:
								for k in node.find_iter_impl(src,bld,dir,accept_name,is_prune,maxdepth=maxdepth-1):
									yield k
		if bld:
			for node in self.childs.values():
				if node.id==bld_ctx.bldnode.id:
					continue
				if node.id&3==BUILD:
					if accept_name(self,node.name):
						yield node
		return
	def find_iter(self,in_pat=['*'],ex_pat=exclude_pats,prune_pat=prune_pats,src=True,bld=True,dir=False,maxdepth=25,flat=False):
		if not(src or bld or dir):
			return
		if self.id&3!=DIR:
			return
		in_pat=Utils.to_list(in_pat)
		ex_pat=Utils.to_list(ex_pat)
		prune_pat=Utils.to_list(prune_pat)
		def accept_name(node,name):
			for pat in ex_pat:
				if fnmatch.fnmatchcase(name,pat):
					return False
			for pat in in_pat:
				if fnmatch.fnmatchcase(name,pat):
					return True
			return False
		def is_prune(node,name):
			for pat in prune_pat:
				if fnmatch.fnmatchcase(name,pat):
					return True
			return False
		ret=self.find_iter_impl(src,bld,dir,accept_name,is_prune,maxdepth=maxdepth)
		if flat:
			return" ".join([x.relpath_gen(self)for x in ret])
		return ret
	def ant_glob(self,*k,**kw):
		src=kw.get('src',1)
		bld=kw.get('bld',0)
		dir=kw.get('dir',0)
		excl=kw.get('excl',exclude_regs)
		incl=k and k[0]or kw.get('incl','**')
		def to_pat(s):
			lst=Utils.to_list(s)
			ret=[]
			for x in lst:
				x=x.replace('//','/')
				if x.endswith('/'):
					x+='**'
				lst2=x.split('/')
				accu=[]
				for k in lst2:
					if k=='**':
						accu.append(k)
					else:
						k=k.replace('.','[.]').replace('*','.*').replace('?','.')
						k='^%s$'%k
						accu.append(re.compile(k))
				ret.append(accu)
			return ret
		def filtre(name,nn):
			ret=[]
			for lst in nn:
				if not lst:
					pass
				elif lst[0]=='**':
					ret.append(lst)
					if len(lst)>1:
						if lst[1].match(name):
							ret.append(lst[2:])
					else:
						ret.append([])
				elif lst[0].match(name):
					ret.append(lst[1:])
			return ret
		def accept(name,pats):
			nacc=filtre(name,pats[0])
			nrej=filtre(name,pats[1])
			if[]in nrej:
				nacc=[]
			return[nacc,nrej]
		def ant_iter(nodi,maxdepth=25,pats=[]):
			nodi.__class__.bld.rescan(nodi)
			for name in nodi.__class__.bld.cache_dir_contents[nodi.id]:
				npats=accept(name,pats)
				if npats and npats[0]:
					accepted=[]in npats[0]
					node=nodi.find_resource(name)
					if node and accepted:
						if src and node.id&3==FILE:
							yield node
					else:
						node=nodi.find_dir(name)
						if node and node.id!=nodi.__class__.bld.bldnode.id:
							if accepted and dir:
								yield node
							if maxdepth:
								for k in ant_iter(node,maxdepth=maxdepth-1,pats=npats):
									yield k
			if bld:
				for node in nodi.childs.values():
					if node.id==nodi.__class__.bld.bldnode.id:
						continue
					if node.id&3==BUILD:
						npats=accept(node.name,pats)
						if npats and npats[0]and[]in npats[0]:
							yield node
			return
		ret=[x for x in ant_iter(self,pats=[to_pat(incl),to_pat(excl)])]
		if kw.get('flat',True):
			return" ".join([x.relpath_gen(self)for x in ret])
		return ret
	def update_build_dir(self,env=None):
		if not env:
			for env in bld.all_envs:
				self.update_build_dir(env)
			return
		path=self.abspath(env)
		lst=Utils.listdir(path)
		try:
			self.__class__.bld.cache_dir_contents[self.id].update(lst)
		except KeyError:
			self.__class__.bld.cache_dir_contents[self.id]=set(lst)
		self.__class__.bld.cache_scanned_folders[self.id]=True
		for k in lst:
			npath=path+os.sep+k
			st=os.stat(npath)
			if stat.S_ISREG(st[stat.ST_MODE]):
				ick=self.find_or_declare(k)
				if not(ick.id in self.__class__.bld.node_sigs[env.variant()]):
					self.__class__.bld.node_sigs[env.variant()][ick.id]=Constants.SIG_NIL
			elif stat.S_ISDIR(st[stat.ST_MODE]):
				child=self.find_dir(k)
				if not child:
					child=self.ensure_dir_node_from_path(k)
				child.update_build_dir(env)
class Nodu(Node):
	pass

class dummy_node_class(Node):
    pass