"""Constrained integer selection weights. No FTC mutation in this solver."""
from pathlib import Path
import json,sys,collections
import numpy as np
from scipy.optimize import milp,Bounds,LinearConstraint
from scipy.sparse import lil_matrix,csr_matrix,vstack,hstack,eye

POS='QB HB FB WR TE LT LG C RG RT LE RE DT LOLB MLB ROLB CB FS SS K P LS'.split()
TARGET=dict(zip(POS,[165,205,5,400,210,104,104,104,104,104,135,135,220,50,135,135,275,135,135,70,70,0]))
base=Path(sys.argv[1]);routes=list(map(int,[x for x in sys.argv[2:]if not x.startswith('--')])) or range(5)

def rational(q):
    best=None
    for b in range(1,1001):
        a=max(1,min(1000,round(q*b/(1-q))))
        key=(abs(a/(a+b)-q),-(a+b),a,b)
        if best is None or key<best:best=key
    return best[2:]

for route in routes:
 m=json.loads((base/f'route{route}_graph.json').read_text())
 cs=[];stars={};before=collections.defaultdict(float);elite=collections.defaultdict(float)
 for bi,b in enumerate(m['branches']):
  for c in b['children']:
   c={**c,'bi':bi};cs.append(c)
   mass=3200*b['weight']/m['pipelineTotal']*c['weight']/b['total']
   for s in c['stars']:
    stars[s['ref']]=s
    v=mass*s['weight']/c['total']
    for p,q in s['position'].items():before[(p,s['tier'])]+=v*q
    if s['tier']>=4:elite[(s['tier'],s['gen'],c['archetype'])]+=v
 totalstars={t:sum(v for (p,s),v in before.items()if s==t)for t in range(1,6)}
 two=3000-1800-totalstars[4]-totalstars[5]
 # Per-position-family nonelite factors keep the absolute elite supply intact.
 groups={tuple(sorted(s['position'])) for s in stars.values()}
 groupbefore={g:{t:sum(before[(p,t)]for p in g)for t in range(1,6)}for g in groups}
 raw3={g:(sum(TARGET[p]for p in g)-v[4]-v[5])*v[3]/(v[2]+v[3])for g,v in groupbefore.items()if v[2]+v[3]}
 k3=1800/sum(raw3.values())
 factors={g:{3:raw3[g]*k3/v[3],2:(sum(TARGET[p]for p in g)-v[4]-v[5]-raw3[g]*k3)/v[2]}for g,v in groupbefore.items()if v[2]+v[3]}
 coefficients=collections.defaultdict(float)
 for c in cs:
  b=m['branches'][c['bi']]
  for z in c['stars']:
   coefficients[z['ref']]+=3200*b['weight']/m['pipelineTotal']*c['weight']/b['total']/c['total']
 sw={r:z['weight']for r,z in stars.items()}
 for g,ff in factors.items():
  for tier,factor in ff.items():
   ids=[r for r,z in stars.items()if tuple(sorted(z['position']))==g and z['tier']==tier and z['weight']]
   goal=groupbefore[g][tier]*factor
   def evaluation(f):return sum(coefficients[r]*min(1000,max(1,round(stars[r]['weight']*f)))for r in ids)
   assert evaluation(1000)>=goal,(g,tier,goal,evaluation(1000))
   lo,hi=0,1000
   for _ in range(60):
    mid=(lo+hi)/2
    if evaluation(mid)<goal:lo=mid
    else:hi=mid
   best=min([lo,hi],key=lambda v:abs(evaluation(v)-goal))
   for r in ids:sw[r]=min(1000,max(1,round(stars[r]['weight']*best)))
 assert all(0<=v<=1000 for v in sw.values())
 # Private nonelite Position overrides avoid changing inherited elite mixes.
 overrides={}
 for group in [('LT','RT'),('LOLB','ROLB')]:
  residual=[TARGET[p]-before[(p,4)]-before[(p,5)]for p in group]
  a,b=rational(residual[0]/sum(residual));overrides['/'.join(group)]={'positions':list(group),'weights':[a,b],'probabilities':[a/(a+b),b/(a+b)]}
 def pos(s):
  if s['tier']in(2,3):
   for spec in overrides.values():
    if set(s['position'])==set(spec['positions']):return dict(zip(spec['positions'],spec['probabilities']))
  return s['position']
 # Preserve the complete geographic/type/star path, not only marginal totals.
 pipeideal=[];desired=[];branchtotals=[]
 for bi,b in enumerate(m['branches']):
  local=np.array([c['weight']*sum(sw[z['ref']]for z in c['stars'])/c['total']for c in cs if c['bi']==bi])
  pipeideal.append(b['weight']*sum(local)/b['total'])
  local*=min(1,995/max(local));desired.extend(local);branchtotals.append(round(sum(local)))
 scale=995/max(pipeideal);pipeweights=np.rint(np.array(pipeideal)*scale).astype(int);pipetotal=sum(pipeweights)
 desired=np.array(desired)
 # Linear constraints use fixed branch totals and fixed star denominators.
 names=[];target=[];tol=[];dictrows=[]
 def row(name,values,value,tolerance):names.append(name);dictrows.append(values);target.append(value);tol.append(tolerance)
 for bi,b in enumerate(m['branches']):row(f'branch:{bi}',{i:1 for i,c in enumerate(cs)if c['bi']==bi},branchtotals[bi],0)
 features=collections.defaultdict(dict)
 for i,c in enumerate(cs):
  b=m['branches'][c['bi']];mass=3000*pipeweights[c['bi']]/pipetotal/branchtotals[c['bi']];den=sum(sw[s['ref']]for s in c['stars'])
  for s in c['stars']:
   v=mass*sw[s['ref']]/den
   for p,q in pos(s).items():
    features[('position',p)][i]=features[('position',p)].get(i,0)+v*q
    if s['tier']>=4:features[('elite_position',p,s['tier'])][i]=features[('elite_position',p,s['tier'])].get(i,0)+v*q
   features[('star',s['tier'])][i]=features[('star',s['tier'])].get(i,0)+v
   if s['tier']>=4:
    key=('elite_leaf',s['tier'],s['gen'],c['archetype']);features[key][i]=features[key].get(i,0)+v
    oldpath=3200*b['weight']/m['pipelineTotal']*c['weight']/b['total']*s['weight']/c['total']
    if oldpath>0:row(f'elite_path:{i}:{s["tier"]}',{i:v},oldpath,.012)
 for p in POS:
  if TARGET[p]:row('position:'+p,features[('position',p)],TARGET[p],.08)
 for key,value in elite.items():
  if value>0:row('elite_leaf:'+str(key),features[('elite_leaf',)+key],value,.004)
 for p in POS:
  for t in (4,5):
   if before[(p,t)]>0:row(f'elite_position:{p}:{t}',features[('elite_position',p,t)],before[(p,t)],.012)
 for t in (2,3,4,5):row(f'star:{t}',features[('star',t)],{2:two,3:1800,4:totalstars[4],5:totalstars[5]}[t],.025)
 n=len(cs);A=lil_matrix((len(names),n))
 for j,r in enumerate(dictrows):
  for i,v in r.items():A[j,i]=v
 A=A.tocsr();target=np.array(target);tol=np.array(tol)
 old=np.array([c['weight']for c in cs],dtype=float)
 # Minimize relative movement around the full-path-preserving fractional recipe.
 C=np.r_[np.zeros(n),1/np.maximum(50,old)]
 M=vstack([hstack([A,csr_matrix((A.shape[0],n))]),hstack([eye(n),-eye(n)]),hstack([-eye(n),-eye(n)])]).tocsr()
 lower=np.r_[target-tol,np.full(2*n,-np.inf)];upper=np.r_[target+tol,desired,-desired]
 bounds=Bounds(np.r_[np.where(old>0,1,0),np.zeros(n)],np.r_[np.where(old>0,1000,0),np.full(n,np.inf)])
 if '--diagnose' in sys.argv:
  vals=A@desired
  print([(name,round(float(v-t),5))for name,t,v in zip(names,target,vals)if name.startswith(('position:','star:'))])
  continue
 result=milp(C,integrality=np.r_[np.ones(n),np.zeros(n)],bounds=bounds,constraints=LinearConstraint(M,lower,upper),options={'time_limit':180,'mip_rel_gap':.05})
 if result.x is None:raise RuntimeError(f'No validated integer solution for route {route}: {result.message}')
 weights=np.rint(result.x[:n]).astype(int);actual=A@weights;error=np.abs(actual-target)
 assert np.max(error-tol)<1e-6,(names[int(np.argmax(error-tol))],np.max(error-tol))
 assert np.all(weights>=bounds.lb[:n])and np.all(weights<=bounds.ub[:n])
 changes={str(r):v for r,v in sw.items()if v!=stars[r]['weight']}
 for b,v in zip(m['branches'],pipeweights):
  if v!=b['weight']:changes[str(b['pipelineRef'])]=int(v)
 for c,v in zip(cs,weights):
  if v!=c['weight']:changes[str(c['ref'])]=int(v)
 out={'route':route,'baseline_sha256':m['baseline_sha256'],'weight_changes':changes,'nonelite_position_overrides':overrides,
      'baseline_stars':totalstars,'target_stars':{1:0,2:two,3:1800,4:totalstars[4],5:totalstars[5]},
      'solver':{'status':int(result.status),'message':str(result.message),'objective':float(result.fun),'feasible_verified':True},
      'constraints':[{'name':name,'target':float(t),'actual':float(a),'error':float(e),'tolerance':float(z)}for name,t,a,e,z in zip(names,target,actual,error,tol)]}
 (base/f'route{route}_solution.json').write_text(json.dumps(out,indent=2)+'\n')
 print(json.dumps({'route':route,'weight_changes':len(changes),'max_position_error':max(e for name,e in zip(names,error)if name.startswith('position:')),'max_elite_leaf_error':max(e for name,e in zip(names,error)if name.startswith('elite_leaf:')),'solver':out['solver']}),flush=True)
